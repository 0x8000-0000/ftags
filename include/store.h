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

   /** Alocates some units of T
    *
    * @param size is the number of units of T
    * @return a pair of key and an interator to the first allocated unit
    */
   std::pair<K, iterator> allocate(std::size_t size);

   /** Releases s units of T allocated at key
    *
    * @param key identifies the block of T
    * @param size is the number of units of T
    * @note this method is optional
    */
   void deallocate(K key, std::size_t size);

   /** Requests access to an allocated block by key
    *
    * @param key identifies the block of T
    * @return a pair of an interator to the first allocated unit and an
    *    iterator to the last accessible unit in the segment
    */
   std::pair<const_iterator, const_iterator> get(K key) const;

   std::pair<iterator, const_iterator> get(K key);

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

} // namespace ftags

#endif // STORE_H_INCLUDED
