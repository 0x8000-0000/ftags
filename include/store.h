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
#include <list>
#include <map>
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

      m_freeBlocks[MaxSegmentSize - 1].push_back(key);
   }

   /** Segment of contiguous T, up to (1<<B) elements in size.
    */
   std::vector<std::vector<T>> m_segment;

   std::map<block_size_type, std::list<K>> m_freeBlocks;
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
   auto suitableBlockIter{m_freeBlocks.lower_bound(size)};

   if ((suitableBlockIter != m_freeBlocks.end()) && (!suitableBlockIter->second.empty()))
   {
      const K key{suitableBlockIter->second.front()};
      suitableBlockIter->second.pop_front();

      const block_size_type segmentIndex{getSegmentIndex(key)};
      const block_size_type offsetInSegment{getOffsetInSegment(key)};

      if (suitableBlockIter->first == size)
      {
         // free block is allocated entirely
      }
      else
      {
         // shrink the leftover block
         const block_size_type leftOverOffsetInSegment{offsetInSegment + size};
         const K               reminderBlockKey{makeKey(segmentIndex, leftOverOffsetInSegment)};
         const block_size_type reminderBlockSize{suitableBlockIter->first - size};
         m_freeBlocks[reminderBlockSize].push_front(reminderBlockKey);
      }

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
      m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const std::pair<block_size_type, std::list<K>>& pp) {
         return std::find(pp.second.begin(), pp.second.end(), candidateKey) != pp.second.end();
      })};
   if (blockIter != m_freeBlocks.end())
   {
      return blockIter->first;
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
      m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const std::pair<block_size_type, std::list<K>>& pp) {
         return std::find(pp.second.begin(), pp.second.end(), candidateKey) != pp.second.end();
      })};
   if (blockIter != m_freeBlocks.end())
   {
      const block_size_type sizeIncrease = newSize - oldSize;
      if (sizeIncrease > blockIter->first)
      {
         // need to call availableAfter first
         throw std::logic_error("Can't allocate more than what's available");
      }

      auto keyIter{std::find(blockIter->second.begin(), blockIter->second.end(), candidateKey)};
      blockIter->second.erase(keyIter);
      if (blockIter->second.empty())
      {
         m_freeBlocks.erase(blockIter);
      }

      if (sizeIncrease == blockIter->first)
      {
         // free block is allocated entirely
      }
      else
      {
         // shrink the leftover block
         const block_size_type leftOverOffsetInSegment{offsetInSegment + newSize};
         const K               reminderBlockKey{makeKey(segmentIndex, leftOverOffsetInSegment)};
         const block_size_type reminderBlockSize{blockIter->first - sizeIncrease};
         m_freeBlocks[reminderBlockSize].push_front(reminderBlockKey);
      }

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

   Block previousBlock{0, 0};
   Block followingBlock{0, 0};

   auto previousEraser = m_freeBlocks.end();
   auto followingEraser = m_freeBlocks.end();

   for (auto groupIter{m_freeBlocks.begin()};
        (groupIter != m_freeBlocks.end()) && (0 == previousBlock.key) && (0 == followingBlock.key);
        ++groupIter)
   {
      for (auto keyIter = groupIter->second.begin(); (keyIter != groupIter->second.end()) && (0 == previousBlock.key);
           ++keyIter)
      {
         const Block oldBlock{*keyIter, groupIter->first};

         if (isAdjacent(oldBlock, newBlock))
         {
            previousBlock = oldBlock;
            groupIter->second.erase(keyIter);
            if (groupIter->second.empty())
            {
               previousEraser = groupIter;
            }
         }
      }

      for (auto keyIter = groupIter->second.begin();
           (keyIter != groupIter->second.end()) && (0 == followingBlock.key);
           ++keyIter)
      {
         const Block oldBlock{*keyIter, groupIter->first};

         if (isAdjacent(newBlock, oldBlock))
         {
            followingBlock = oldBlock;
            groupIter->second.erase(keyIter);
            if (groupIter->second.empty())
            {
               followingEraser = groupIter;
            }
         }
      }
   }

   if (previousEraser == followingEraser)
   {
      if (previousEraser != m_freeBlocks.end())
      {
         m_freeBlocks.erase(previousEraser);
      }
   }
   else
   {
      if (previousEraser != m_freeBlocks.end())
      {
         m_freeBlocks.erase(previousEraser);
      }

      if (followingEraser != m_freeBlocks.end())
      {
         m_freeBlocks.erase(followingEraser);
      }
   }

   if ((0 == previousBlock.key) && (0 == followingBlock.key))
   {
      m_freeBlocks[size].push_back(key);
   }
   else if ((0 != previousBlock.key) && (0 != followingBlock.key))
   {
      // adjacent both left and right; credit all size to previous and delete following
      assert(previousBlock.key < newBlock.key);
      assert(newBlock.key < followingBlock.key);

      m_freeBlocks[previousBlock.size + size + followingBlock.size].push_back(previousBlock.key);
   }
   else
   {
      if (0 != previousBlock.key)
      {
         assert(previousBlock.key < newBlock.key);
         m_freeBlocks[previousBlock.size + size].push_back(previousBlock.key);
      }
      else
      {
         assert(newBlock.key < followingBlock.key);
         m_freeBlocks[size + followingBlock.size].push_back(newBlock.key);
      }
   }
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::validateInternalState() const
{
#ifdef FTAGS_STRICT_CHECKING
#if 0
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
#endif
}

} // namespace ftags

#endif // STORE_H_INCLUDED
