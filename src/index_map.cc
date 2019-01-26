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
   validateInternalState();

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
   auto           sizeCapacityIter{iter};
   const uint32_t capacity{*sizeCapacityIter >> 16};
   const uint32_t size{*sizeCapacityIter & 0xffff};
   assert(size <= capacity);

   iter++; // iter now points to first value

   if (size < capacity)
   {
      *sizeCapacityIter = (capacity << 16) | (size + 1);

      std::advance(iter, size);
      *iter = value;

      validateInternalState();
      return;
   }

   /*
    * no more room in this bag; need to grow
    */
   bag_size_type newCapacity{nextCapacity(capacity)};

   /*
    * first see if there is a next bag (then if we can push it out of the way)
    */
   auto nextBagIter{iter};
   std::advance(nextBagIter, size);
   if (nextBagIter != segmentEnd)
   {
      /*
       * there is another bag after this one
       */
      uint32_t nextBagKey{*nextBagIter};
      nextBagIter++; // now points to size / capacity
      assert(nextBagIter != segmentEnd);
      const uint32_t nextBagCapacity{*nextBagIter >> 16};
#ifndef NDEBUG
      const uint32_t nextBagSize{*nextBagIter & 0xffff};
#endif
      assert(nextBagSize <= nextBagCapacity);
      nextBagIter++; // now points to next bag data
      assert(nextBagIter != segmentEnd);

      if ((nextBagKey != 0) && (nextBagCapacity <= newCapacity))
      {
         /*
          * there is a next bag, and it is smaller than the intended capacity
          * of the current bag; move it out of the way and take over its space
          */
         auto nextBagIndexPos{m_index.find(nextBagKey)};
         assert(nextBagIndexPos != m_index.end());
         const auto nextBagStorageKey{nextBagIndexPos->second};
         reallocateBag(nextBagKey, nextBagCapacity, nextBagStorageKey, nextBagIter);

         // now indicate that there is no next bag, because we just moved it out
         nextBagKey = 0;
      }

      if (nextBagKey == 0)
      {
         /*
          * we can expand into the area; ask the store to allocate specific block
          */

         bag_size_type available{nextBagCapacity + MetadataSize};
         if (nextBagCapacity == 0)
         {
            // clean memory so we need to ask store
            available = m_store.availableAfter(storageKey, capacity + MetadataSize);
            assert(available != 0);
         }

         /*
          * can grow in place
          */
         if ((newCapacity - capacity) >= available)
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

         validateInternalState();
         return;
      }
   }

   /*
    * move this bag to a new location
    */
   reallocateBag(key, newCapacity, storageKey, iter);

   validateInternalState();

   add(key, value);
}

ftags::IndexMap::iterator ftags::IndexMap::allocateBag(uint32_t key, bag_size_type capacity, bag_size_type size)
{
   auto location{m_store.allocate(capacity + MetadataSize)};
   auto iter{location.iterator};

   *iter = key;

   iter++; // iter now points to size / capacity
   *iter = (static_cast<uint32_t>(capacity) << 16) | static_cast<uint32_t>(size);

   // record new location for bag
   m_index[key] = location.key;

   iter++; // iter now points to first value

#ifndef NDEBUG
   std::fill_n(iter, capacity, 0);
#endif

   return iter;
}

ftags::IndexMap::iterator ftags::IndexMap::reallocateBag(uint32_t                  key,
                                                         bag_size_type             capacity,
                                                         uint32_t                  oldStorageKey,
                                                         ftags::IndexMap::iterator oldData)
{
   auto copyIter{oldData};

   // reset old bag metadata
   oldData--; // now points to the old size / capacity
   const uint32_t oldCapacity{*oldData >> 16};
   uint32_t oldSize{*oldData & 0xffff};
   assert(oldSize <= oldCapacity);
   *oldData = oldCapacity << 16;
   oldData--; // now points to the old key
   *oldData = 0;

   // assert(capacity != size); // move as-is for now
   auto iter{allocateBag(key, capacity, oldSize)};

   // copy the data elements
   std::copy_n(copyIter, oldSize, iter);

   std::fill_n(copyIter, oldSize, 0);

   // release old bag
   m_store.deallocate(oldStorageKey, oldCapacity + MetadataSize);

   // advance iterator after the moved data
   std::advance(iter, oldSize);
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
      auto iter{location.first};
#ifndef NDEBUG
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);
#endif

      // verify key
      assert(key == *iter);

      // advance iter to size / capacity
      iter++;
      auto sizeCapacityIter{iter};
#ifndef NDEBUG
      uint32_t capacity{*sizeCapacityIter >> 16};
#endif
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
      auto iter{location.first};
#ifndef NDEBUG
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);
#endif

      // verify key
      assert(key == *iter);

      *iter = 0; // erase key

      iter++; // iter now points to size / capacity

      uint32_t capacity{*iter >> 16};
      uint32_t size{*iter & 0xffff};
      assert(size <= capacity);

      *iter = (capacity << 16); // reset size; leave capacity

      // reset content
      std::fill_n(iter, size, 0);

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
      auto iter{location.first};
#ifndef NDEBUG
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);
#endif

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

#ifdef FTAGS_STRICT_CHECKING
void ftags::IndexMap::validateInternalState() const
{
   m_store.validateInternalState();

   std::size_t capacityAllocated = 0;

   for (const auto& bag : m_index)
   {
#ifndef NDEBUG
      const uint32_t key{bag.first};
#endif

      auto location{m_store.get(bag.second)};
      // unpack location
      auto       iter{location.first};
      const auto segmentEnd{location.second};
      assert(iter != segmentEnd);

      // verify key
      assert(key == *iter);

      iter++; // iter now points to size / capacity
      auto     sizeCapacityIter{iter};
      uint32_t capacity{*sizeCapacityIter >> 16};
#ifndef NDEBUG
      uint32_t size{*sizeCapacityIter & 0xffff};
      assert(size <= capacity);
#endif

      iter++; // iter now points to first value

      capacityAllocated += capacity;
   }
}
#endif
