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

#include <flat_map.h>
#include <store.h>

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <cstddef>
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
   StringTable(bool enableConcurrentAccess = false) : m_useSafeConcurrentAccess{enableConcurrentAccess}
   {
   }

   StringTable(const StringTable& other) :
      m_store{other.m_store},
      m_index{other.m_index},
      m_useSafeConcurrentAccess{other.m_useSafeConcurrentAccess}
   {
   }

   StringTable(StringTable&& other) :
      m_store{std::move(other.m_store)},
      m_index{std::move(other.m_index)},
      m_useSafeConcurrentAccess{other.m_useSafeConcurrentAccess}
   {
   }

   StringTable& operator=(const StringTable& other)
   {
      if (this != &other)
      {
         m_store = other.m_store;
         m_index = other.m_index;
      }
      return *this;
   }

   StringTable& operator=(StringTable&& other)
   {
      if (this != &other)
      {
         m_store = std::move(other.m_store);
         m_index = std::move(other.m_index);
      }

      return *this;
   }

   bool operator==(const StringTable& other) const
   {
      if (this == &other)
      {
         return true;
      }

      for (const auto& [key, val] : m_index)
      {
         if (other.m_index.find(key) == other.m_index.end())
         {
            return false;
         }
      }

      for (const auto& [key, val] : other.m_index)
      {
         if (m_index.find(key) == m_index.end())
         {
            return false;
         }
      }

      return true;
   }

   using Key = uint32_t;

   const char* getString(Key stringKey) const noexcept;

   std::size_t getSize() const
   {
      return m_index.size();
   }

   Key  getKey(const char* string) const noexcept;
   Key  addKey(const char* string);
   void removeKey(const char* string);

   /* Add all the keys from other that are missing in this table.
    *
    * @return a mapping between the keys in the old table and the keys in the updated table
    */
   ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key> mergeStringTable(const StringTable& other);

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static StringTable deserialize(ftags::BufferExtractor& extractor);

private:
   // not-thread safe method
   Key insertString(const char* aString);

   static constexpr uint32_t k_bucketSize = 24;

   using StoreType = ftags::Store<char, Key, k_bucketSize>;

   StoreType m_store;

   /*
    * Lookup table; can be reconstructed from the store above.
    */
   std::unordered_map<const char*, Key, CharPointerHashingFunctor, CharPointerCompareFunctor> m_index;

   const bool                m_useSafeConcurrentAccess;
   mutable std::shared_mutex m_mutex;
};

} // namespace ftags

#endif // STRING_TABLE_H_INCLUDED
