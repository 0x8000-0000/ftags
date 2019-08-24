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
 * std::multimap<uint32_t, uint32_t>
 */

template <>
std::size_t ftags::Serializer<std::multimap<uint32_t, uint32_t>>::computeSerializedSize(
   const std::multimap<uint32_t, uint32_t>& val)
{
   using map_uu = std::multimap<uint32_t, uint32_t>;

   static_assert(sizeof(map_uu::key_type) == sizeof(uint32_t));
   static_assert(sizeof(map_uu::mapped_type) == sizeof(uint32_t));

   return sizeof(ftags::SerializedObjectHeader) + sizeof(map_uu::size_type) +
          val.size() * (sizeof(map_uu::key_type) + sizeof(map_uu::mapped_type));
}

template <>
void ftags::Serializer<std::multimap<uint32_t, uint32_t>>::serialize(const std::multimap<uint32_t, uint32_t>& val,
                                                                     ftags::BufferInsertor& insertor)
{
   ftags::SerializedObjectHeader header = {};
   insertor << header;

   insertor << val.size();

   for (const auto& iter : val)
   {
      insertor << iter.first << iter.second;
   }
}

template <>
std::multimap<uint32_t, uint32_t>
ftags::Serializer<std::multimap<uint32_t, uint32_t>>::deserialize(ftags::BufferExtractor& extractor)
{
   std::multimap<uint32_t, uint32_t> retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   std::multimap<uint32_t, uint32_t>::size_type mapSize = 0;
   extractor >> mapSize;

   for (std::multimap<uint32_t, uint32_t>::size_type ii = 0; ii < mapSize; ii++)
   {
      uint32_t key   = 0;
      uint32_t value = 0;

      extractor >> key >> value;

      retval.emplace(key, value);
   }

   return retval;
}

/*
 * std::vector<char>
 */

template <>
std::size_t ftags::Serializer<std::vector<char>>::computeSerializedSize(const std::vector<char>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(std::vector<char>::size_type) + val.size();
}

template <>
void ftags::Serializer<std::vector<char>>::serialize(const std::vector<char>& val, ftags::BufferInsertor& insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<char>"};
   insertor << header;

   insertor << val.size();

   insertor << val;
}

template <>
std::vector<char> ftags::Serializer<std::vector<char>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   std::vector<char>::size_type vecSize = 0;
   extractor >> vecSize;

   std::vector<char> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}

/*
 * std::vector<uint32_t>
 */

template <>
std::size_t ftags::Serializer<std::vector<uint32_t>>::computeSerializedSize(const std::vector<uint32_t>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(std::vector<uint32_t>::size_type) +
          val.size() * sizeof(uint32_t);
}

template <>
void ftags::Serializer<std::vector<uint32_t>>::serialize(const std::vector<uint32_t>& val,
                                                         ftags::BufferInsertor&       insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<uint32_t>"};
   insertor << header;

   insertor << val.size();

   insertor << val;
}

template <>
std::vector<uint32_t> ftags::Serializer<std::vector<uint32_t>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   std::vector<uint32_t>::size_type vecSize = 0;
   extractor >> vecSize;

   std::vector<uint32_t> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}

/*
 * std::vector<uint64_t>
 */

template <>
std::size_t ftags::Serializer<std::vector<uint64_t>>::computeSerializedSize(const std::vector<uint64_t>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(std::vector<uint64_t>::size_type) +
          val.size() * sizeof(uint64_t);
}

template <>
void ftags::Serializer<std::vector<uint64_t>>::serialize(const std::vector<uint64_t>& val,
                                                         ftags::BufferInsertor&       insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<uint64_t>"};
   insertor << header;

   insertor << val.size();

   insertor << val;
}

template <>
std::vector<uint64_t> ftags::Serializer<std::vector<uint64_t>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   std::vector<uint64_t>::size_type vecSize = 0;
   extractor >> vecSize;

   std::vector<uint64_t> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}

/*
 * std::string
 */
template <>
std::size_t ftags::Serializer<std::string>::computeSerializedSize(const std::string& val)
{
   return sizeof(uint64_t) + val.size();
}

template <>
void ftags::Serializer<std::string>::serialize(const std::string& val, ftags::BufferInsertor& insertor)
{
   const uint64_t size = val.size();
   insertor << size;
   insertor.serialize(val.data(), size);
}

template <>
std::string ftags::Serializer<std::string>::deserialize(ftags::BufferExtractor& extractor)
{
   uint64_t size = 0;
   extractor >> size;
   std::string retval(/* n = */ size, /* c = */ '\0');
   extractor.deserialize(retval.data(), size);
   return retval;
}
