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

#ifndef SERIALIZATION_H_INCLUDED
#define SERIALIZATION_H_INCLUDED

#include <cstddef>
#include <cstdint>

namespace ftags
{

struct SerializedObjectHeader
{
   // 128 bit hash of the rest of the header + body; uses SpookyV2 Hash
   uint64_t m_hash[2];

   // object name or uuid or whatever
   char m_objectType[16];

   // the version of serialization for this type
   uint64_t m_version;

   // 64 bit object size
   uint64_t m_size;
};

struct Serializable
{
   /*
    * Serialization interface
    */

   std::size_t computeSerializedSize() const;

   std::size_t serialize(std::byte* buffer, std::size_t size) const;

};

} // namespace ftags

#endif // SERIALIZATION_H_INCLUDED
