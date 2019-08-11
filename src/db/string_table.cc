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

const char* ftags::StringTable::getString(Key stringKey) const noexcept
{
   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock_shared();
   }

   auto location = m_store.get(stringKey);

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock_shared();
   }

   if (location.first == location.second)
   {
      return nullptr;
   }
   else
   {
      return &*location.first;
   }
}

ftags::StringTable::Key ftags::StringTable::getKey(const char* inputString) const noexcept
{
   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock_shared();
   }

   auto iter = m_index.find(inputString);

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock_shared();
   }

   if (m_index.end() == iter)
   {
      return 0;
   }
   else
   {
      return iter->second;
   }
}

ftags::StringTable::Key ftags::StringTable::addKey(const char* inputString)
{
   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock_shared();
   }

   const Key currentPosition{getKey(inputString)};

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock_shared();
   }

   if (currentPosition != 0)
   {
      return currentPosition;
   }

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock();
   }

   Key key = insertString(inputString);

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock();
   }

   return key;
}

ftags::StringTable::Key ftags::StringTable::insertString(const char* aString)
{
   const auto inputLength{static_cast<uint32_t>(strlen(aString))};

   // allocate extra byte for NUL
   auto allocation{m_store.allocate(inputLength + 1)};

   // also copy NUL
   std::copy_n(aString, inputLength + 1, allocation.iterator);

   m_index[&*allocation.iterator] = allocation.key;

   return allocation.key;
}

void ftags::StringTable::removeKey(const char* inputString)
{
   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock_shared();
   }

   auto iter = m_index.find(inputString);

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock_shared();
   }

   if (m_index.end() == iter)
   {
      return;
   }

   const auto inputLength{static_cast<uint32_t>(strlen(inputString))};

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock();
   }

#ifndef NDEBUG
   auto buffer = m_store.get(iter->second).first;
   std::fill_n(buffer, inputLength + 1, '\0');
#endif

   // allocated extra byte for NUL
   m_store.deallocate(iter->second, inputLength + 1);

   m_index.erase(iter);

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock();
   }
}

ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key> ftags::StringTable::mergeStringTable(const StringTable& other)
{
   ftags::FlatMapAccumulator<ftags::StringTable::Key, ftags::StringTable::Key> accumulator(other.m_index.size());

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock();
   }

   for (const auto& pair: other.m_index)
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

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock();
   }

   return accumulator.getMap();
}
