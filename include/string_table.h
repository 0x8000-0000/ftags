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

class StringTable
{
   static constexpr unsigned BucketSizeBits = 24;
   static constexpr size_t   MaxTableSize   = (1u << BucketSizeBits);
   static constexpr size_t   MaxBucketCount = (1u << (32 - BucketSizeBits));

public:
   StringTable()
   {
      addBucket();
   }

   const char* getString(uint32_t stringKey) const noexcept;

   uint32_t getKey(const char* string) const noexcept;
   uint32_t addKey(const char* string);

   std::vector<uint8_t> serialize() const;

   static StringTable deserialize(const uint8_t* buffer, size_t size);

private:
   void addBucket();

   /*
    * Store the strings in continuous ropes; in order to reduce memory
    * fragmentation, store the strings contiguously in up to (1 << 8) buckets of
    * (1 << 24) bytes each. The key is formed by (bucket id << 24) + offset in
    * bucket. The string NUL-terminator also serves as separator.
    */
   std::vector<std::vector<char>> m_buckets;

   /*
    * Lookup table; can be reconstructed from the buckets above.
    */
   std::unordered_map<const char*, uint32_t, CharPointerHashingFunctor, CharPointerCompareFunctor> m_registry;
};

} // namespace ftags

#endif // STRING_TABLE_H_INCLUDED
