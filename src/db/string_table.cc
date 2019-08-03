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

   const auto inputLength{static_cast<uint32_t>(strlen(inputString))};

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.lock();
   }

   // allocate extra bite for NUL
   auto allocation{m_store.allocate(inputLength + 1)};

   // also copy NUL
   std::copy_n(inputString, inputLength + 1, allocation.iterator);

   m_index[&*allocation.iterator] = allocation.key;

   if (m_useSafeConcurrentAccess)
   {
      m_mutex.unlock();
   }

   return allocation.key;
}

