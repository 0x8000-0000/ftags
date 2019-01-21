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
   auto indexPos = m_index.find(key);

   if (indexPos != m_index.end())
   {
      auto location = m_store.get(indexPos->second);
      assert(location.first != location.second);

      auto iter = location.first;

      assert(key == *iter);
      iter++;

      uint32_t capacity = *iter >> 16;
      uint32_t size     = *iter & 0xffff;
      assert(size <= capacity);

      if (size < capacity)
      {
         *iter = (capacity << 16) | (size + 1);

         std::advance(iter, size + 1);
         *iter = value;
      }
      else
      {
         // need to grow
         assert(false);
      }
   }
   else
   {
      uint32_t capacity = InitialAllocationSize;
      auto     location = m_store.allocate(2 + capacity);
      auto     iter     = location.second;
      *iter             = key;
      iter++;
      *iter = (capacity << 16) | (1);
      iter++;
      *iter        = value;
      m_index[key] = location.first;
   }
}

std::pair<ftags::IndexMap::const_iterator, ftags::IndexMap::const_iterator>
ftags::IndexMap::getValues(uint32_t key) const noexcept
{
   auto keyPos = m_index.find(key);
   if (keyPos != m_index.end())
   {
      auto location = m_store.get(keyPos->second);

      assert(location.first != location.second);

      auto iter = location.first;

      assert(key == *iter);
      iter++;

      uint32_t capacity = *iter >> 16;
      uint32_t size     = *iter & 0xffff;
      assert(size <= capacity);

      iter++;

      auto begin = iter;
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
   // TODO: deallocate
   m_index.erase(key);
}

void ftags::IndexMap::removeValue(uint32_t key, uint32_t value)
{
   auto indexPos = m_index.find(key);
   if (indexPos != m_index.end())
   {
      const auto location = m_store.get(indexPos->second);
      assert(location.first != location.second);

      auto iter = location.first;

      assert(key == *iter);
      iter++;

      auto sizeCapacityIter = iter;

      uint32_t capacity = *iter >> 16;
      uint32_t size     = *iter & 0xffff;
      assert(size <= capacity);

      iter++;

      auto rangeEnd = iter;
      std::advance(rangeEnd, size);

      auto pos = std::find(iter, rangeEnd, value);
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
