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
   using iterator       = typename std::vector<T>::iterator;
   using const_iterator = typename std::vector<T>::const_iterator;

   Store()
   {
      addSegment();
   }

   /** Allocates some units of T
    *
    * @param size is the number of units of T
    * @return a pair of key and an iterator to the first allocated unit
    */
   std::pair<K, iterator> allocate(std::size_t size);

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
   void deallocate(K key, std::size_t size);

   /** Return the number of units we know can be allocated starting at key
    *
    * @param key identifies a block
    * @param size is the number of units of T
    *
    * @note This succeeds if a block starting at K is in the free list or
    * if K is right at the end of the current allocation.
    * @see allocateAt
    */
   std::size_t availableAfter(K key, std::size_t size) const noexcept;

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
   iterator allocateAfter(K key, std::size_t oldSize, std::size_t newSize);

private:
   static constexpr std::size_t MaxSegmentSize      = (1UL << SegmentSizeBits);
   static constexpr std::size_t MaxSegmentCount     = (1UL << ((sizeof(K) * 8) - SegmentSizeBits));
   static constexpr std::size_t OffsetInSegmentMask = (MaxSegmentSize - 1);
   static constexpr std::size_t SegmentIndexMask    = ((MaxSegmentCount - 1) << SegmentSizeBits);

   static std::size_t getOffsetInSegment(K key)
   {
      return (key & OffsetInSegmentMask);
   }

   static std::size_t getSegmentIndex(K key)
   {
      return (key >> SegmentSizeBits) & (MaxSegmentCount - 1);
   }

   static K makeKey(std::size_t segmentIndex, std::size_t offsetInSegment)
   {
      assert(segmentIndex < MaxSegmentCount);
      assert(offsetInSegment < MaxSegmentSize);

      return static_cast<K>((segmentIndex << SegmentSizeBits) | offsetInSegment);
   }

   void addSegment()
   {
      if ((m_segment.size() + 1) >= MaxSegmentCount)
      {
         throw std::length_error("Exceeded data structure capacity");
      }

      m_segment.emplace_back(std::vector<T>());
      m_segment.back().reserve(MaxSegmentSize);
      m_segment.back().push_back(T());
   }

   /** Segment of contiguous T, up to (1<<B) elements in size.
    */
   std::vector<std::vector<T>> m_segment;

   std::vector<std::pair<K, std::size_t>> m_freeBlocks;
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

   const std::size_t segmentIndex    = getSegmentIndex(key);
   const std::size_t offsetInSegment = getOffsetInSegment(key);

   const auto& segment = m_segment.at(segmentIndex);
   auto        iter    = segment.begin();
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

   const std::size_t segmentIndex    = getSegmentIndex(key);
   const std::size_t offsetInSegment = getOffsetInSegment(key);

   auto& segment = m_segment.at(segmentIndex);
   auto  iter    = segment.begin();
   std::advance(iter, offsetInSegment);

   assert(iter < segment.end());

   return std::pair<typename Store<T, K, SegmentSizeBits>::iterator,
                    typename Store<T, K, SegmentSizeBits>::const_iterator>(iter, segment.end());
}

template <typename T, typename K, unsigned SegmentSizeBits>
std::pair<K, typename Store<T, K, SegmentSizeBits>::iterator> Store<T, K, SegmentSizeBits>::allocate(std::size_t size)
{
   if (size >= MaxSegmentSize)
   {
      throw std::length_error("Can't store objects that large");
   }

   auto& currentSegment = m_segment.back();

   if ((currentSegment.size() + size) <= currentSegment.capacity())
   {
      const std::ptrdiff_t segmentIndex = std::distance(m_segment.begin(), m_segment.end()) - 1;
      assert(segmentIndex >= 0);
      const std::size_t offsetInSegment = currentSegment.size();
      currentSegment.resize(currentSegment.size() + size);

      const K key = makeKey(static_cast<size_t>(segmentIndex), offsetInSegment);

      auto iter = currentSegment.begin();
      std::advance(iter, offsetInSegment);

      // TODO: consider adding the reminder of this segment to the free list

      return std::pair<K, typename Store<T, K, SegmentSizeBits>::iterator>(key, iter);
   }
   else
   {
      addSegment();

      return allocate(size);
   }
}

template <typename T, typename K, unsigned SegmentSizeBits>
std::size_t Store<T, K, SegmentSizeBits>::availableAfter(K key, std::size_t size) const noexcept
{
   if (key == 0)
   {
      return 0;
   }

   const std::size_t segmentIndex    = getSegmentIndex(key);
   const std::size_t offsetInSegment = getOffsetInSegment(key);

   const auto& segment = m_segment.at(segmentIndex);

   if ((offsetInSegment + size) == segment.size())
   {
      return segment.capacity() - (offsetInSegment + size);
   }

   const K candidateKey{makeKey(segmentIndex, offsetInSegment + size)};

   auto blockIter{std::find_if(
      m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const std::pair<K, std::size_t>& pp) { return pp.first == candidateKey; })};
   if (blockIter != m_freeBlocks.end())
   {
      return blockIter->second;
   }

   return 0;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::iterator
Store<T, K, SegmentSizeBits>::allocateAfter(K key, std::size_t oldSize, std::size_t newSize)
{
   if (key == 0)
   {
      throw std::invalid_argument("Key 0 is invalid");
   }

   const std::size_t segmentIndex    = getSegmentIndex(key);
   const std::size_t offsetInSegment = getOffsetInSegment(key);

   auto& segment = m_segment.at(segmentIndex);

   if ((offsetInSegment + oldSize) == segment.size())
   {
      if ((offsetInSegment + newSize) <= segment.capacity())
      {
         auto iter = segment.end();
         segment.resize(offsetInSegment + newSize);
         return iter;
      }
      else
      {
         throw std::length_error("Can't extend using requested size");
      }
   }
   else
   {
      const K candidateKey{makeKey(segmentIndex, offsetInSegment + oldSize)};

      auto blockIter{std::remove_if(
         m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const std::pair<K, std::size_t>& pp) { return pp.first == candidateKey; })};
      if (blockIter != m_freeBlocks.end())
      {
         if (newSize != (oldSize + blockIter->second))
         {
            throw std::logic_error("Can't allocate less than the entire block");
         }

         m_freeBlocks.erase(blockIter, m_freeBlocks.end());

         auto iter{segment.begin()};
         std::advance(iter, offsetInSegment + oldSize);

         return iter;
      }

      throw std::logic_error("Can't extend allocation; this is not the last element");
   }
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::deallocate(K key, std::size_t size)
{
   // leak it
   m_freeBlocks.emplace_back(key, size);
}

} // namespace ftags

#endif // STORE_H_INCLUDED
