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

#include <serialization.h>

#include <map>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

/*
 * std::map<uint32_t, uint32_t>
 */

template <>
std::size_t
ftags::Serializer<std::map<uint32_t, uint32_t>>::computeSerializedSize(const std::map<uint32_t, uint32_t>& val)
{
   using map_uu = std::map<uint32_t, uint32_t>;

   static_assert(sizeof(map_uu::key_type) == sizeof(uint32_t));
   static_assert(sizeof(map_uu::mapped_type) == sizeof(uint32_t));

   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) +
          val.size() * (sizeof(map_uu::key_type) + sizeof(map_uu::mapped_type));
}

template <>
std::size_t ftags::Serializer<std::map<uint32_t, uint32_t>>::serialize(const std::map<uint32_t, uint32_t>& val,
                                                                       std::byte*                          buffer,
                                                                       std::size_t                         size)
{
   const auto originalBuffer = buffer;

   ftags::SerializedObjectHeader header = {};
   std::memcpy(buffer, &header, sizeof(header));
   buffer += sizeof(header);

   const uint64_t mapSize = val.size();
   std::memcpy(buffer, &mapSize, sizeof(mapSize));
   buffer += sizeof(mapSize);

   for (const auto& iter : val)
   {
      std::memcpy(buffer, &iter.first, sizeof(iter.first));
      buffer += sizeof(iter.first);

      std::memcpy(buffer, &iter.second, sizeof(iter.second));
      buffer += sizeof(iter.second);
   }

   const auto used = static_cast<size_t>(std::distance(originalBuffer, buffer));
   assert(used == size);

   return used;
}

template <>
std::map<uint32_t, uint32_t> ftags::Serializer<std::map<uint32_t, uint32_t>>::deserialize(const std::byte* buffer,
                                                                                          std::size_t      size)
{
   const auto originalBuffer = buffer;

   std::map<uint32_t, uint32_t> retval;

   ftags::SerializedObjectHeader header = {};
   std::memcpy(&header, buffer, sizeof(header));
   buffer += sizeof(header);

   uint64_t mapSize = 0;
   std::memcpy(&mapSize, buffer, sizeof(mapSize));
   buffer += sizeof(mapSize);

   for (uint64_t ii = 0; ii < mapSize; ii++)
   {
      uint32_t key   = 0;
      uint32_t value = 0;

      std::memcpy(&key, buffer, sizeof(key));
      buffer += sizeof(key);

      std::memcpy(&value, buffer, sizeof(value));
      buffer += sizeof(value);

      retval[key] = value;
   }

   const auto used = static_cast<size_t>(std::distance(originalBuffer, buffer));
   assert(used == size);

   return retval;
}

/*
 * std::vector<char>
 */

template <>
std::size_t ftags::Serializer<std::vector<char>>::computeSerializedSize(const std::vector<char>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) + val.size();
}

template <>
std::size_t ftags::Serializer<std::vector<char>>::serialize(const std::vector<char>& val, std::byte* buffer, std::size_t size)
{
   const auto originalBuffer = buffer;

   ftags::SerializedObjectHeader header = {};
   std::memcpy(buffer, &header, sizeof(header));
   buffer += sizeof(header);

   const uint64_t vecSize = val.size();
   std::memcpy(buffer, &vecSize, sizeof(vecSize));
   buffer += sizeof(vecSize);

   std::memcpy(buffer, val.data(), vecSize);
   buffer += vecSize;

   const auto used = static_cast<size_t>(std::distance(originalBuffer, buffer));
   assert(used == size);

   return used;
}

template <>
std::vector<char> ftags::Serializer<std::vector<char>>::deserialize(const std::byte* buffer, std::size_t size)
{
   const auto originalBuffer = buffer;

   ftags::SerializedObjectHeader header = {};
   std::memcpy(&header, buffer, sizeof(header));
   buffer += sizeof(header);

   uint64_t vecSize = 0;
   std::memcpy(&vecSize, buffer, sizeof(vecSize));
   buffer += sizeof(vecSize);

   std::vector<char> retval(/* size = */ vecSize);
   std::memcpy(retval.data(), buffer, vecSize);
   buffer += vecSize;

   const auto used = static_cast<size_t>(std::distance(originalBuffer, buffer));
   assert(used == size);

   return retval;
}
