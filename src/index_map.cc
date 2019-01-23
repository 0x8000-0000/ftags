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

#include <index_map.h>

#include <algorithm>

void ftags::IndexMap::add(uint32_t key, uint32_t value)
{
   auto indexPos{m_index.find(key)};
   if (indexPos == m_index.end())
   {
      /*
       * first time we see this key; allocate new bag
       */
      auto iter{allocateBag(key, InitialAllocationSize, 1)};
      *iter = value;

      return;
   }

   const auto storageKey{indexPos->second};
   auto       location{m_store.get(storageKey)};
   // unpack location
   auto       iter{location.first};
   const auto segmentEnd{location.second};
   assert(iter != segmentEnd);

   // verify key
   assert(key == *iter);

   iter++; // iter now points to size / capacity
   auto     sizeCapacityIter{iter};
   uint32_t capacity{*sizeCapacityIter >> 16};
   uint32_t size{*sizeCapacityIter & 0xffff};
   assert(size <= capacity);

   iter++; // iter now points to first value

   if (size < capacity)
   {
      *sizeCapacityIter = (capacity << 16) | (size + 1);

      std::advance(iter, size);
      *iter = value;

      return;
   }

   /*
    * no more room in this bag; need to grow
    */
   std::size_t newCapacity{capacity};
   if ((capacity / GrowthFactor) < GrowthFactor)
   {
      newCapacity += GrowthFactor;
   }
   else
   {
      newCapacity += capacity / GrowthFactor;
   }

   const std::size_t available{m_store.availableAfter(storageKey, capacity + MetadataSize)};
   if (available != 0)
   {
      /*
       * can grow in place
       */
      if ((newCapacity - capacity) > available)
      {
         newCapacity = capacity + available;
      }

      if ((available - (newCapacity - capacity)) <= (InitialAllocationSize + MetadataSize))
      {
         newCapacity = capacity + available;
      }

      // TODO: implement chaining if we need more than 1<<16 elements
      assert(newCapacity <= (1 << 16));

      *sizeCapacityIter = (static_cast<uint32_t>(newCapacity) << 16) | (size + 1);

      auto extraUnits{m_store.extend(storageKey, capacity + MetadataSize, newCapacity + MetadataSize)};
      *extraUnits = value;

      return;
   }

   /*
    * cannot grow in place, for one of the two reasons
    * 1. this is the last bag in the segment and its extent is right at the end
    * 2. there is another bag after this one
    */

   /*
    * first see if there is a next bag (then if we can push it out of the way)
    */
   auto nextBagIter{iter};
   std::advance(nextBagIter, size);
   if (nextBagIter == segmentEnd)
   {
      /*
       * this is the last bag in this segment
       */

      // TODO: implement chaining if we need more than 1<<16 elements
      assert(newCapacity <= (1 << 16));

      /*
       * move this bag to a new location
       */
      auto newIter{reallocateBag(key, newCapacity, size + 1, storageKey, iter)};

      // save the new value
      *newIter = value;

      return;
   }

   /*
    * there is another bag after this one
    */

   uint32_t nextBagKey{*nextBagIter};
   assert(nextBagKey != 0);

   nextBagIter++; // now points to size / capacity
   uint32_t nextBagCapacity{*nextBagIter >> 16};
   uint32_t nextBagSize{*nextBagIter & 0xffff};
   assert(nextBagSize <= nextBagCapacity);

   nextBagIter++; // now points to next bag data

   if (nextBagCapacity <= newCapacity)
   {
      /*
       * next bag is smaller than the intended capacity of the current bag
       * move it out of the way and take over its space
       */

      auto nextBagIndexPos{m_index.find(nextBagKey)};
      assert(nextBagIndexPos != m_index.end());
      const auto nextBagStorageKey{nextBagIndexPos->second};

      reallocateBag(nextBagKey, nextBagCapacity, nextBagSize, nextBagStorageKey, nextBagIter);

      *sizeCapacityIter = (static_cast<uint32_t>(capacity + nextBagCapacity + MetadataSize) << 16) | (size + 1);

      // move to the end of the current bag
      std::advance(iter, size);

      // save the new value
      *iter = value;

      return;
   }

   /*
    * move this bag to a new location
    */
   auto newIter{reallocateBag(key, newCapacity, size + 1, storageKey, iter)};

   // save the new value
   *newIter = value;
}

ftags::IndexMap::iterator ftags::IndexMap::allocateBag(uint32_t key, std::size_t capacity, std::size_t size)
{
   auto location{m_store.allocate(capacity + MetadataSize)};
   auto iter{location.iterator};

   *iter = key;

   iter++; // iter now points to size / capacity
   *iter = (static_cast<uint32_t>(capacity) << 16) | static_cast<uint32_t>(size);

   // record new location for bag
   m_index[key] = location.key;

   iter++; // iter now points to first value
   return iter;
}

ftags::IndexMap::iterator ftags::IndexMap::reallocateBag(
   uint32_t key, std::size_t capacity, std::size_t size, uint32_t oldStorageKey, ftags::IndexMap::iterator oldData)
{
   auto iter{allocateBag(key, capacity, size)};

   // copy the data elements
   std::copy_n(oldData, size, iter);

   // reset old bag metadata
   oldData--; // now points to the old size / capacity
   *oldData = 0;
   oldData--; // now points to the old key
   *oldData = 0;

   // release old bag
   m_store.deallocate(oldStorageKey, capacity + MetadataSize);

   // advance iterator after the moved data
   std::advance(iter, size);
   return iter;
}

std::pair<ftags::IndexMap::const_iterator, ftags::IndexMap::const_iterator>
ftags::IndexMap::getValues(uint32_t key) const noexcept
{
   auto indexPos{m_index.find(key)};
   if (indexPos != m_index.end())
   {
      const auto storageKey{indexPos->second};
      auto       location{m_store.get(storageKey)};
      // unpack location
      auto       iter{location.first};
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);

      // verify key
      assert(key == *iter);

      // advance iter to size / capacity
      iter++;
      auto     sizeCapacityIter{iter};
      uint32_t capacity{*sizeCapacityIter >> 16};
      uint32_t size{*sizeCapacityIter & 0xffff};
      assert(size <= capacity);

      // advance iter to first value
      iter++;

      auto begin{iter};
      std::advance(iter, size);

      return std::pair<ftags::IndexMap::const_iterator, ftags::IndexMap::const_iterator>(begin, iter);
   }
   else
   {
      return m_store.get(0);
   }
}

void ftags::IndexMap::removeKey(uint32_t key)
{
   auto indexPos{m_index.find(key)};
   if (indexPos != m_index.end())
   {
      const auto storageKey{indexPos->second};
      const auto location{m_store.get(storageKey)};
      // unpack location
      auto       iter{location.first};
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);

      // verify key
      assert(key == *iter);

      *iter = 0; // erase key

      iter++; // iter now points to size / capacity

      uint32_t capacity{*iter >> 16};
      uint32_t size{*iter & 0xffff};
      assert(size <= capacity);

      *iter = 0; // erase size / capacity

      // deallocate
      m_store.deallocate(storageKey, capacity + MetadataSize);

      m_index.erase(indexPos);
   }
}

void ftags::IndexMap::removeValue(uint32_t key, uint32_t value)
{
   auto indexPos{m_index.find(key)};
   if (indexPos != m_index.end())
   {
      const auto storageKey{indexPos->second};
      const auto location{m_store.get(storageKey)};
      // unpack location
      auto       iter{location.first};
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);

      // verify key
      assert(key == *iter);

      // advance iter to size / capacity
      iter++;
      auto sizeCapacityIter{iter};

      uint32_t capacity{*iter >> 16};
      uint32_t size{*iter & 0xffff};
      assert(size <= capacity);

      // advance iter to first value
      iter++;

      auto rangeEnd{iter};
      std::advance(rangeEnd, size);

      auto pos{std::find(iter, rangeEnd, value)};
      if (pos != rangeEnd)
      {
         rangeEnd--;

         /* micro-optimization: swap victim with last element and remove last
          * element; calling erase could move half the array which is wasteful
          * since we don't care about the order of the objects
          */
         *pos = *rangeEnd;

         *sizeCapacityIter = (capacity << 16) | (size - 1);
      }
   }
}
