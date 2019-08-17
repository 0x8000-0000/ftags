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
void ftags::Serializer<std::map<uint32_t, uint32_t>>::serialize(const std::map<uint32_t, uint32_t>& val,
                                                                ftags::BufferInsertor&              insertor)
{
   ftags::SerializedObjectHeader header = {};
   insertor << header;

   const uint64_t mapSize = val.size();
   insertor << mapSize;

   for (const auto& iter : val)
   {
      insertor << iter.first << iter.second;
   }
}

template <>
std::map<uint32_t, uint32_t>
ftags::Serializer<std::map<uint32_t, uint32_t>>::deserialize(ftags::BufferExtractor& extractor)
{
   std::map<uint32_t, uint32_t> retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   uint64_t mapSize = 0;
   extractor >> mapSize;

   for (uint64_t ii = 0; ii < mapSize; ii++)
   {
      uint32_t key   = 0;
      uint32_t value = 0;

      extractor >> key >> value;

      retval[key] = value;
   }

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
void ftags::Serializer<std::vector<char>>::serialize(const std::vector<char>& val, ftags::BufferInsertor& insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<char>"};
   insertor << header;

   const uint64_t vecSize = val.size();
   insertor << vecSize;

   insertor << val;
}

template <>
std::vector<char> ftags::Serializer<std::vector<char>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   uint64_t vecSize = 0;
   extractor >> vecSize;

   std::vector<char> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}

/*
 * std::vector<uint64_t>
 */

template <>
std::size_t ftags::Serializer<std::vector<uint64_t>>::computeSerializedSize(const std::vector<uint64_t>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) + val.size();
}

template <>
void ftags::Serializer<std::vector<uint64_t>>::serialize(const std::vector<uint64_t>& val,
                                                         ftags::BufferInsertor&       insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<uint64_t>"};
   insertor << header;

   const uint64_t vecSize = val.size();
   insertor << vecSize;

   insertor << val;
}

template <>
std::vector<uint64_t> ftags::Serializer<std::vector<uint64_t>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   uint64_t vecSize = 0;
   extractor >> vecSize;

   std::vector<uint64_t> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}
