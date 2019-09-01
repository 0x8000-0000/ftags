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

const char* ftags::util::StringTable::getString(Key stringKey) const noexcept
{
   auto location = m_store.get(stringKey);

   if (location.first == location.second)
   {
      return nullptr;
   }
   else
   {
      return &*location.first;
   }
}

ftags::util::StringTable::Key ftags::util::StringTable::getKey(const char* inputString) const noexcept
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

ftags::util::StringTable::Key ftags::util::StringTable::addKey(const char* inputString)
{
   const Key currentPosition{getKey(inputString)};

   if (currentPosition != 0)
   {
      return currentPosition;
   }

   Key key = insertString(inputString);

   return key;
}

ftags::util::StringTable::Key ftags::util::StringTable::insertString(const char* aString)
{
   const auto inputLength{static_cast<uint32_t>(strlen(aString))};

   // allocate extra byte for NUL
   auto allocation{m_store.allocate(inputLength + 1)};

   // also copy NUL
   std::copy_n(aString, inputLength + 1, allocation.iterator);

   m_index[&*allocation.iterator] = allocation.key;

   return allocation.key;
}

void ftags::util::StringTable::removeKey(const char* inputString)
{
   auto iter = m_index.find(inputString);

   if (m_index.end() == iter)
   {
      return;
   }

   const auto inputLength{static_cast<uint32_t>(strlen(inputString))};

#ifndef NDEBUG
   auto buffer = m_store.get(iter->second).first;
   std::fill_n(buffer, inputLength + 1, '\0');
#endif

   // allocated extra byte for NUL
   m_store.deallocate(iter->second, inputLength + 1);

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
