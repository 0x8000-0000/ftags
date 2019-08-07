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

#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

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

class BufferInsertor
{
public:
   BufferInsertor(std::byte* buffer, std::size_t size) : m_buffer{buffer}, m_size{size}
   {
   }

   BufferInsertor(std::vector<std::byte>& buffer) : m_buffer{buffer.data()}, m_size{buffer.size()}
   {
   }

   template <typename T>
   BufferInsertor& operator<<(const T& value)
   {
#ifndef NDEBUG
      assert(sizeof(value) <= m_size);
      m_size -= sizeof(value);
#endif
      std::memcpy(m_buffer, &value, sizeof(value));
      m_buffer += sizeof(value);

      return *this;
   }

   template <typename T>
   BufferInsertor& operator<<(const std::vector<T>& value)
   {
      const std::size_t size = value.size() * sizeof(T);
#ifndef NDEBUG
      assert(size <= m_size);
      m_size -= size;
#endif
      std::memcpy(m_buffer, value.data(), size);
      m_buffer += size;

      return *this;
   }

   void assertEmpty()
   {
#ifndef NDEBUG
      assert(m_size == 0);
#else
      (void)m_size;
#endif
   }

   std::byte* getBuffer()
   {
      return m_buffer;
   }

private:
   std::byte*  m_buffer;
   std::size_t m_size;
};

class BufferExtractor
{
public:
   BufferExtractor(const std::byte* buffer, std::size_t size) : m_buffer{buffer}, m_size{size}
   {
   }

   BufferExtractor(std::vector<std::byte>& buffer) : m_buffer{buffer.data()}, m_size{buffer.size()}
   {
   }

   template <typename T>
   BufferExtractor& operator>>(T& value)
   {
#ifndef NDEBUG
      assert(sizeof(value) <= m_size);
      m_size -= sizeof(value);
#endif
      std::memcpy(&value, m_buffer, sizeof(value));
      m_buffer += sizeof(value);

      return *this;
   }

   template <typename T>
   BufferExtractor& operator>>(std::vector<T>& value)
   {
      const std::size_t size = value.size() * sizeof(T);
#ifndef NDEBUG
      assert(size <= m_size);
      m_size -= size;
#endif
      std::memcpy(value.data(), m_buffer, size);
      m_buffer += size;

      return *this;
   }

   void assertEmpty()
   {
#ifndef NDEBUG
      assert(m_size == 0);
#else
      (void)m_size;
#endif
   }

   const std::byte* getBuffer()
   {
      return m_buffer;
   }

private:
   const std::byte* m_buffer;
   std::size_t      m_size;
};

template <typename T>
struct Serializer
{
   /*
    * Serialization interface
    */

   static std::size_t computeSerializedSize(const T& t);

   static void serialize(const T& t, BufferInsertor& BufferInsertor);

   static T deserialize(BufferExtractor& bufferExtractor);
};


} // namespace ftags

#endif // SERIALIZATION_H_INCLUDED