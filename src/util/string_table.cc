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

#include <string_table.h>

#include <algorithm>
#include <stdexcept>

#include <cassert>
#include <cstring>

ftags::util::StringTable::Key ftags::util::StringTable::getKey(std::string_view inputString) const noexcept
{
   auto iter = m_index.find(inputString);

   if (m_index.end() == iter)
   {
      return 0;
   }
   else
   {
      return iter->second;
   }
}

ftags::util::StringTable::Key ftags::util::StringTable::addKey(std::string_view inputString) noexcept
{
   const Key currentPosition{getKey(inputString)};

   if (currentPosition != 0)
   {
      return currentPosition;
   }

   Key key = insertString(inputString);

   return key;
}

ftags::util::StringTable::Key ftags::util::StringTable::insertString(std::string_view string) noexcept
{
   // allocate extra byte for NUL
   auto allocation{m_store.allocate(static_cast<StoreType::block_size_type>(string.size() + 1))};

   std::copy_n(string.data(), string.size(), allocation.iterator);

   // also copy NUL
   allocation.iterator[string.size()] = '\0';

   m_index[allocation.iterator] = allocation.key;

   return allocation.key;
}

void ftags::util::StringTable::removeKey(std::string_view inputString) noexcept
{
   auto iter = m_index.find(inputString);

   if (m_index.end() == iter)
   {
      return;
   }

#ifndef NDEBUG
   auto buffer = m_store.get(iter->second).first;
   std::fill_n(buffer, inputString.size() + 1, '\0');
#endif

   // allocated extra byte for NUL
   m_store.deallocate(iter->second, static_cast<StoreType::block_size_type>(inputString.size() + 1));

   m_index.erase(iter);
}

ftags::util::FlatMap<ftags::util::StringTable::Key, ftags::util::StringTable::Key>
ftags::util::StringTable::mergeStringTable(const StringTable& other)
{
   FlatMapAccumulator<StringTable::Key, StringTable::Key> accumulator(other.m_index.size());

   for (const auto& pair : other.m_index)
   {
      auto iter = m_index.find(pair.first);
      if (iter == m_index.end())
      {
         accumulator.add(pair.second, insertString(pair.first));
      }
      else
      {
         accumulator.add(pair.second, iter->second);
      }
   }

   return accumulator.getMap();
}
