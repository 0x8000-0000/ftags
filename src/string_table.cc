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

const char* ftags::StringTable::getString(uint32_t stringKey) const noexcept
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

uint32_t ftags::StringTable::getKey(const char* inputString) const noexcept
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

uint32_t ftags::StringTable::addKey(const char* inputString)
{
   const uint32_t currentPosition{getKey(inputString)};
   if (currentPosition != 0)
   {
      return currentPosition;
   }

   const auto inputLength{static_cast<uint32_t>(strlen(inputString))};

   auto allocation{m_store.allocate(inputLength + 1)};

   std::copy_n(inputString, inputLength + 1, allocation.iterator);

   m_index[inputString] = allocation.key;

   return allocation.key;
}

std::vector<uint8_t> ftags::StringTable::serialize() const
{
   std::vector<uint8_t> retval;
   return retval;
}

ftags::StringTable ftags::StringTable::deserialize(const uint8_t* /* buffer */, size_t /* size */)
{
   ftags::StringTable retval;
   return retval;
}
