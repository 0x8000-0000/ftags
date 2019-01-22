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

#ifndef STRING_TABLE_H_INCLUDED
#define STRING_TABLE_H_INCLUDED

#include <store.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <cstring>

namespace ftags
{

class CharPointerHashingFunctor
{
public:
   size_t operator()(const char* key) const
   {
      /*
       * lazy implementation for now
       */
      const std::string asString{key};
      return std::hash<std::string>()(asString);
   }
};

class CharPointerCompareFunctor
{
public:
   bool operator()(const char* leftString, const char* rightString) const
   {
      return 0 == strcmp(leftString, rightString);
   }
};

/** Space optimized and serializable symbol table
 *
 * Conceptually, this contains a std::map<std::string, uint32_t> and a
 * std::vector<std::string> where the contents of the map is the index where
 * a string is stored. So given either an id or a string you can efficiently
 * retrieve the other.
 */
class StringTable
{
public:
   const char* getString(uint32_t stringKey) const noexcept;

   uint32_t getKey(const char* string) const noexcept;
   uint32_t addKey(const char* string);
   void removeKey(const char* string);

   /*
    * Serialization interface
    */

   std::vector<uint8_t> serialize() const;

   static StringTable deserialize(const uint8_t* buffer, size_t size);

private:
   ftags::Store<char, uint32_t, 24> m_store;

   /*
    * Lookup table; can be reconstructed from the store above.
    */
   std::unordered_map<const char*, uint32_t, CharPointerHashingFunctor, CharPointerCompareFunctor> m_index;
};

} // namespace ftags

#endif // STRING_TABLE_H_INCLUDED
