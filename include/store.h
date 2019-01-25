/*
   Copyright 2019 Florin Iucha

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef STORE_H_INCLUDED
#define STORE_H_INCLUDED

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <vector>

#include <cassert>

namespace ftags
{

/** Allocates blocks of T, returning a cookie
 *
 * The store only guarantees that successive allocations do not overlap. The
 * store does not track the allocations in any way, other than to manage and
 * recycle the free blocks.
 *
 * The cookie acts similarly to a synthetic pointer, but it can be converted
 * to the underlying iterator via an explicit request only.
 */
template <typename T, typename K, unsigned SegmentSizeBits = 24>
class Store
{

public:
   using iterator        = typename std::vector<T>::iterator;
   using const_iterator  = typename std::vector<T>::const_iterator;
   using block_size_type = uint32_t;

   struct Allocation
   {
      K                                 key;
      typename std::vector<T>::iterator iterator;
   };

   Store()
   {
      addSegment();
   }

   /** Allocates some units of T
    *
    * @param size is the number of units of T
    * @return a pair of key and an iterator to the first allocated unit
    */
   Allocation allocate(block_size_type size);

   /** Requests access to an allocated block by key
    *
    * @param key identifies the block of T
    * @return a pair of an iterator to the first allocated unit and an
    *    iterator to the last accessible unit in the segment
    */
   std::pair<const_iterator, const_iterator> get(K key) const;

   std::pair<iterator, const_iterator> get(K key);

   /** Releases s units of T allocated at key
    *
    * @param key identifies the block of T
    * @param size is the number of units of T
    * @note this method is optional
    */
   void deallocate(K key, block_size_type size);

   /** Return the number of units we know can be allocated starting at key
    *
    * @param key identifies a block
    * @param size is the number of units of T
    *
    * @note This succeeds if a block starting at K is in the free list or
    * if K is right at the end of the current allocation.
    * @see allocateAt
    */
   block_size_type availableAfter(K key, block_size_type size) const noexcept;

   /** Extends the block at key K
    *
    * @param key identifies the block of T
    * @param oldSize is the number of units of T currently allocated
    * @param newSize is the number of units of T desired
    *
    * @note For this operation to succeed, availableAt(K, size) should return
    * more than newSize - size
    * @see availableAt
    */
   iterator extend(K key, block_size_type oldSize, block_size_type newSize);

   /** Runs a self-check
    */
   void validateInternalState() const;

private:
   static constexpr block_size_type MaxSegmentSize      = (1U << SegmentSizeBits);
   static constexpr block_size_type MaxSegmentCount     = (1U << ((sizeof(K) * 8) - SegmentSizeBits));
   static constexpr block_size_type OffsetInSegmentMask = (MaxSegmentSize - 1);
   static constexpr block_size_type SegmentIndexMask    = ((MaxSegmentCount - 1) << SegmentSizeBits);

   static block_size_type getOffsetInSegment(K key)
   {
      return (key & OffsetInSegmentMask);
   }

   static block_size_type getSegmentIndex(K key)
   {
      return (key >> SegmentSizeBits) & (MaxSegmentCount - 1);
   }

   struct Block
   {
      K               key;
      block_size_type size;
   };

   static bool isAdjacent(const Block& left, const Block& right)
   {
      const block_size_type leftSegmentIndex{getSegmentIndex(left.key)};
      const block_size_type rightSegmentIndex{getSegmentIndex(right.key)};

      if (leftSegmentIndex != rightSegmentIndex)
      {
         return false;
      }

      const block_size_type leftOffsetInSegment{getOffsetInSegment(left.key)};
      const block_size_type rightOffsetInSegment{getOffsetInSegment(right.key)};

      return (leftOffsetInSegment + left.size) == rightOffsetInSegment;
   }

   static K makeKey(block_size_type segmentIndex, block_size_type offsetInSegment)
   {
      assert(segmentIndex < MaxSegmentCount);
      assert(offsetInSegment < MaxSegmentSize);

      return static_cast<K>((segmentIndex << SegmentSizeBits) | offsetInSegment);
   }

   void addSegment()
   {
      const block_size_type segmentsInUse{static_cast<block_size_type>(m_segment.size())};

      if ((segmentsInUse + 1) >= MaxSegmentCount)
      {
         throw std::length_error("Exceeded data structure capacity");
      }

      m_segment.emplace_back(std::vector<T>());
      m_segment.back().resize(MaxSegmentSize);

      K key{makeKey(segmentsInUse, 1)};

      const Block newBlock{key, MaxSegmentSize - 1};

      m_freeBlocks.push_back(newBlock);
   }

   /** Segment of contiguous T, up to (1<<B) elements in size.
    */
   std::vector<std::vector<T>> m_segment;

   std::vector<Block> m_freeBlocks;
};

/*
 * implementation details
 */
template <typename T, typename K, unsigned SegmentSizeBits>
std::pair<typename Store<T, K, SegmentSizeBits>::const_iterator,
          typename Store<T, K, SegmentSizeBits>::const_iterator>
Store<T, K, SegmentSizeBits>::get(K key) const
{
   if (key == 0)
   {
      return std::pair<typename Store<T, K, SegmentSizeBits>::const_iterator,
                       typename Store<T, K, SegmentSizeBits>::const_iterator>(m_segment[0].end(), m_segment[0].end());
   }

   const block_size_type segmentIndex{getSegmentIndex(key)};
   const block_size_type offsetInSegment{getOffsetInSegment(key)};

   const auto& segment{m_segment.at(segmentIndex)};
   auto        iter{segment.begin()};
   std::advance(iter, offsetInSegment);

   assert(iter < segment.end());

   return std::pair<typename Store<T, K, SegmentSizeBits>::const_iterator,
                    typename Store<T, K, SegmentSizeBits>::const_iterator>(iter, segment.end());
}

template <typename T, typename K, unsigned SegmentSizeBits>
std::pair<typename Store<T, K, SegmentSizeBits>::iterator, typename Store<T, K, SegmentSizeBits>::const_iterator>
Store<T, K, SegmentSizeBits>::get(K key)
{
   if (key == 0)
   {
      return std::pair<typename Store<T, K, SegmentSizeBits>::iterator,
                       typename Store<T, K, SegmentSizeBits>::const_iterator>(m_segment[0].end(), m_segment[0].end());
   }

   const block_size_type segmentIndex    = getSegmentIndex(key);
   const block_size_type offsetInSegment = getOffsetInSegment(key);

   auto& segment = m_segment.at(segmentIndex);
   auto  iter    = segment.begin();
   std::advance(iter, offsetInSegment);

   assert(iter < segment.end());

   return std::pair<typename Store<T, K, SegmentSizeBits>::iterator,
                    typename Store<T, K, SegmentSizeBits>::const_iterator>(iter, segment.end());
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::Allocation Store<T, K, SegmentSizeBits>::allocate(block_size_type size)
{
   if (size >= MaxSegmentSize)
   {
      throw std::length_error("Can't store objects that large");
   }

   /*
    * check if there is a suitable free block available
    */
   auto suitableBlockIter{std::lower_bound(
      m_freeBlocks.begin(), m_freeBlocks.end(), size, [](const Block& pp, block_size_type ss) -> bool {
         return pp.size < ss;
      })};

   if (suitableBlockIter != m_freeBlocks.end())
   {
      assert(suitableBlockIter->size >= size);

      const K key{suitableBlockIter->key};

      const block_size_type segmentIndex{getSegmentIndex(key)};
      const block_size_type offsetInSegment{getOffsetInSegment(key)};

      if (suitableBlockIter->size == size)
      {
         // free block is allocated entirely
         m_freeBlocks.erase(suitableBlockIter);
      }
      else
      {
         // shrink the leftover block
         const block_size_type leftOverOffsetInSegment{offsetInSegment + size};
         suitableBlockIter->key = makeKey(segmentIndex, leftOverOffsetInSegment);
         suitableBlockIter->size -= size;
      }

      // sort free blocks by size so we can quickly find a suitable block
      std::sort(m_freeBlocks.begin(), m_freeBlocks.end(), [](const Block& left, const Block& right) -> bool {
         return left.size < right.size;
      });

      auto iter{m_segment[segmentIndex].begin()};
      std::advance(iter, offsetInSegment);

      return Allocation{key, iter};
   }

   /*
    * no free block of the requisite size
    */

   addSegment();

   return allocate(size);
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::block_size_type
Store<T, K, SegmentSizeBits>::availableAfter(K key, typename Store<T, K, SegmentSizeBits>::block_size_type size) const
   noexcept
{
   if (key == 0)
   {
      return 0;
   }

   const block_size_type segmentIndex{getSegmentIndex(key)};
   const block_size_type offsetInSegment{getOffsetInSegment(key)};

   const K candidateKey{makeKey(segmentIndex, offsetInSegment + size)};

   auto blockIter{std::find_if(
      m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const Block& pp) { return pp.key == candidateKey; })};
   if (blockIter != m_freeBlocks.end())
   {
      return blockIter->size;
   }

   return 0;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::iterator
Store<T, K, SegmentSizeBits>::extend(K key, block_size_type oldSize, block_size_type newSize)
{
   if (key == 0)
   {
      throw std::invalid_argument("Key 0 is invalid");
   }

   if (oldSize == newSize)
   {
      throw std::invalid_argument("Nothing to extend; old size and new size are the same");
   }

   const block_size_type segmentIndex    = getSegmentIndex(key);
   const block_size_type offsetInSegment = getOffsetInSegment(key);

   const K candidateKey{makeKey(segmentIndex, offsetInSegment + oldSize)};

   auto blockIter{std::find_if(
      m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const Block& pp) { return pp.key == candidateKey; })};
   if (blockIter != m_freeBlocks.end())
   {
      if (newSize > (oldSize + blockIter->size))
      {
         throw std::logic_error("Can't allocate more than what's available");
      }

      const block_size_type sizeIncrease = newSize - oldSize;

      if (sizeIncrease == blockIter->size)
      {
         // free block is allocated entirely
         m_freeBlocks.erase(blockIter);
      }
      else
      {
         // shrink the leftover block
         const block_size_type leftOverOffsetInSegment{offsetInSegment + newSize};
         blockIter->key = makeKey(segmentIndex, leftOverOffsetInSegment);
         blockIter->size -= sizeIncrease;
      }

      // sort free blocks by size so we can quickly find a suitable block
      std::sort(m_freeBlocks.begin(), m_freeBlocks.end(), [](const Block& left, const Block& right) -> bool {
         return left.size < right.size;
      });

      auto& segment = m_segment.at(segmentIndex);
      auto  iter{segment.begin()};
      std::advance(iter, offsetInSegment + oldSize);

      return iter;
   }
   else
   {
      throw std::logic_error("Can't extend allocation; no free block follows");
   }
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::deallocate(K key, block_size_type size)
{
   const Block newBlock{key, size};

   /*
    * coalesce with adjacent blocks
    */
   auto previousAdjacentBlock{std::find_if(
      m_freeBlocks.begin(), m_freeBlocks.end(), [&newBlock](const Block& pp) { return isAdjacent(pp, newBlock); })};
   auto followingAdjacentBlock{std::find_if(
      m_freeBlocks.begin(), m_freeBlocks.end(), [&newBlock](const Block& pp) { return isAdjacent(newBlock, pp); })};

   if ((previousAdjacentBlock == m_freeBlocks.end()) && (followingAdjacentBlock == m_freeBlocks.end()))
   {
      // not adjacent to any free blocks; just add it
      m_freeBlocks.push_back(newBlock);
   }
   else
   {
      if ((previousAdjacentBlock != m_freeBlocks.end()) && (followingAdjacentBlock != m_freeBlocks.end()))
      {
         // adjacent both left and right; credit all size to previous and delete following
         previousAdjacentBlock->size += size + followingAdjacentBlock->size;

         m_freeBlocks.erase(followingAdjacentBlock);
      }
      else
      {
         if (previousAdjacentBlock != m_freeBlocks.end())
         {
            previousAdjacentBlock->size += size;
         }
         else
         {
            assert(followingAdjacentBlock != m_freeBlocks.end());
            followingAdjacentBlock->key = key;
            followingAdjacentBlock->size += size;
         }
      }
   }

   // sort free blocks by size so we can quickly find a suitable block
   std::sort(m_freeBlocks.begin(), m_freeBlocks.end(), [](const Block& left, const Block& right) -> bool {
      return left.size < right.size;
   });
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::validateInternalState() const
{
#ifdef FTAGS_STRICT_CHECKING
   std::vector<Block> freeBlocks(m_freeBlocks);

   std::sort(freeBlocks.begin(), freeBlocks.end(), [](const Block& left, const Block& right) -> bool {
      return left.key < right.key;
   });

   block_size_type currentSegment{MaxSegmentCount + 1};
   block_size_type currentOffset{0};

   for (const auto& block : freeBlocks)
   {
      const block_size_type segmentIndex{getSegmentIndex(block.key)};
      const block_size_type offsetInSegment{getOffsetInSegment(block.key)};

      if (currentSegment != segmentIndex)
      {
         currentSegment = segmentIndex;
         currentOffset  = 0;
      }
      else
      {
         assert(currentOffset <= offsetInSegment);
         if (currentOffset > offsetInSegment)
         {
            throw std::logic_error("Internal free blocks corruption");
         }

         currentOffset = offsetInSegment + block.size;
      }
   }

#endif
}

} // namespace ftags

#endif // STORE_H_INCLUDED
