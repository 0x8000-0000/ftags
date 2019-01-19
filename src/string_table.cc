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

const char* ftags::StringTable::getString(uint32_t /* stringKey */)
{
   return nullptr;
}

uint32_t ftags::StringTable::getKey(const char* /* string */)
{
   return 0;
}

uint32_t ftags::StringTable::addKey(const char* /* string */)
{
   return 0;
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
