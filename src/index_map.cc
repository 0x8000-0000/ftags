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
   m_index[key].push_back(value);
}

std::vector<uint32_t> ftags::IndexMap::getValues(uint32_t key) const noexcept
{
   auto values = m_index.find(key);
   if (values != m_index.end())
   {
      return values->second;
   }
   else
   {
      return std::vector<uint32_t>();
   }
}

void ftags::IndexMap::removeKey(uint32_t key)
{
   m_index.erase(key);
}

void ftags::IndexMap::removeValue(uint32_t key, uint32_t value)
{
   auto values = m_index.find(key);
   if (values != m_index.end())
   {
      auto& bag = values->second;
      auto pos = std::find(bag.begin(), bag.end(), value);
      if (pos != bag.end())
      {
         /* micro-optimization: swap victim with last element and remove last
          * element; calling erase could move half the array which is wasteful
          * since we don't care about the order of the objects
          */
         *pos = bag.back();
         bag.pop_back();
      }
   }
}
