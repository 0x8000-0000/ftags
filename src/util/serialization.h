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

#include <algorithm>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ftags::util
{

struct SerializedObjectHeader
{
   // 128 bit hash of the rest of the header + body; uses SpookyV2 Hash
   uint64_t m_hash[2] = {}; // NOLINT

   // object name or uuid or whatever
   char m_objectType[16] = {}; // NOLINT

   // the version of serialization for this type
   uint64_t m_version = 1; // NOLINT

   // 64 bit object size
   uint64_t m_size = 0; // NOLINT

   explicit SerializedObjectHeader(std::string_view name)
   {
      size_t len = name.size();
      if (len >= (sizeof(m_objectType) - 1))
      {
         len = sizeof(m_objectType) - 1;
      }
      std::copy_n(name.data(), len, m_objectType);
      std::fill_n(m_objectType + len, sizeof(m_objectType) - len, '\0');
   }

   SerializedObjectHeader()
   {
      std::fill_n(m_objectType, sizeof(m_objectType), '\0');
   }
};

class SerializationWriter // NOLINT
{
public:
   virtual ~SerializationWriter() = default;

   virtual void serialize(const char* data, std::size_t byteSize) = 0;

#ifndef NDEBUG
   virtual void assertEmpty() = 0;
#else
   void assertEmpty()
   {
   }
#endif
};

class SerializationReader // NOLINT
{
public:
   virtual ~SerializationReader() = default;

   virtual void deserialize(char* data, std::size_t byteSize) = 0;

#ifndef NDEBUG
   virtual void assertEmpty() = 0;
#else
   void assertEmpty()
   {
   }
#endif
};

class BufferSerializationWriter : public SerializationWriter
{
public:
   BufferSerializationWriter(std::byte* buffer, std::size_t size) : m_buffer{buffer}, m_size{size}
   {
      if (m_buffer == nullptr)
      {
         throw(std::logic_error("Invalid buffer address"));
      }

      if (m_size == 0)
      {
         throw(std::logic_error("Invalid buffer size"));
      }
   }

   explicit BufferSerializationWriter(std::vector<std::byte>& buffer) : m_buffer{buffer.data()}, m_size{buffer.size()}
   {
      if (m_buffer == nullptr)
      {
         throw(std::logic_error("Invalid buffer address"));
      }

      if (m_size == 0)
      {
         throw(std::logic_error("Invalid buffer size"));
      }
   }

   void serialize(const char* data, std::size_t byteSize) override
   {
      assert(byteSize <= m_size);
      m_size -= byteSize;
      std::memcpy(m_buffer, data, byteSize);
      m_buffer += byteSize;
   }

#ifndef NDEBUG
   void assertEmpty() override
   {
      assert(m_size == 0);
   }
#endif

   std::byte* getBuffer()
   {
      return m_buffer;
   }

private:
   std::byte*  m_buffer;
   std::size_t m_size;
};

class BufferSerializationReader : public SerializationReader
{
public:
   BufferSerializationReader(const std::byte* buffer, std::size_t size) : m_buffer{buffer}, m_size{size}
   {
      if (buffer == nullptr)
      {
         throw(std::logic_error("Invalid serialization buffer"));
      }
   }

   explicit BufferSerializationReader(std::vector<std::byte>& buffer) : m_buffer{buffer.data()}, m_size{buffer.size()}
   {
   }

   void deserialize(char* data, std::size_t byteSize) override
   {
      assert(byteSize <= m_size);
      m_size -= byteSize;
      std::memcpy(data, m_buffer, byteSize);
      m_buffer += byteSize;
   }

#ifndef NDEBUG
   void assertEmpty() override
   {
      assert(m_size == 0);
   }
#endif

   const std::byte* getBuffer()
   {
      return m_buffer;
   }

private:
   const std::byte* m_buffer;
   std::size_t      m_size;
};

class TypedInsertor
{
public:
   explicit TypedInsertor(SerializationWriter& writer) : m_writer{writer}
   {
   }

   TypedInsertor& serialize(const char* data, std::size_t byteSize)
   {
      m_writer.serialize(data, byteSize);

      return *this;
   }

   template <typename T>
   TypedInsertor& operator<<(const T& value)
   {
      return serialize(static_cast<const char*>(static_cast<const void*>(&value)), sizeof(T));
   }

   template <typename T>
   TypedInsertor& operator<<(const std::vector<T>& value)
   {
      return serialize(value, value.size());
   }

   template <typename T>
   TypedInsertor& serialize(const std::vector<T>& value, std::size_t size)
   {
      assert(size <= value.size());
      const std::size_t byteSize = size * sizeof(T);
      return serialize(static_cast<const char*>(static_cast<const void*>(value.data())), byteSize);
   }

   void assertEmpty()
   {
      m_writer.assertEmpty();
   }

private:
   SerializationWriter& m_writer;
};

class TypedExtractor
{
public:
   explicit TypedExtractor(SerializationReader& reader) : m_reader{reader}
   {
   }

   TypedExtractor& deserialize(void* data, std::size_t byteSize)
   {
      m_reader.deserialize(static_cast<char*>(data), byteSize);

      return *this;
   }

   template <typename T>
   TypedExtractor& operator>>(T& value)
   {
      deserialize(static_cast<void*>(&value), sizeof(T));

      return *this;
   }

   template <typename T>
   TypedExtractor& operator>>(std::vector<T>& value)
   {
      deserialize(value, value.size());

      return *this;
   }

   template <typename T>
   TypedExtractor& deserialize(std::vector<T>& value, std::size_t size)
   {
      assert(size <= value.size());
      const std::size_t byteSize = size * sizeof(T);
      deserialize(static_cast<char*>(static_cast<void*>(value.data())), byteSize);

      return *this;
   }

   void assertEmpty()
   {
      m_reader.assertEmpty();
   }

private:
   SerializationReader& m_reader;
};

/*
 * legacy implementation
 */

class BufferInsertor
{
public:
   BufferInsertor(std::byte* buffer, std::size_t size) : m_writer{buffer, size}, m_insertor{m_writer}
   {
   }

   explicit BufferInsertor(std::vector<std::byte>& buffer) : m_writer{buffer}, m_insertor{m_writer}
   {
   }

   TypedInsertor& getInsertor()
   {
      return m_insertor;
   }

   void serialize(const void* data, std::size_t byteSize)
   {
      m_insertor.serialize(static_cast<const char*>(data), byteSize);
   }

   template <typename T>
   BufferInsertor& operator<<(const T& value)
   {
      m_insertor << value;

      return *this;
   }

   template <typename T>
   BufferInsertor& operator<<(const std::vector<T>& value)
   {
      serialize(value, value.size());

      return *this;
   }

   template <typename T>
   void serialize(const std::vector<T>& value, std::size_t size)
   {
      m_insertor.serialize(value, size);
   }

   void assertEmpty()
   {
      m_insertor.assertEmpty();
   }

   std::byte* getBuffer()
   {
      return m_writer.getBuffer();
   }

private:
   BufferSerializationWriter m_writer;
   TypedInsertor             m_insertor;
};

class BufferExtractor
{
public:
   BufferExtractor(const std::byte* buffer, std::size_t size) : m_reader{buffer, size}, m_extractor{m_reader}
   {
   }

   explicit BufferExtractor(std::vector<std::byte>& buffer) : m_reader{buffer}, m_extractor{m_reader}
   {
   }

   TypedExtractor& getExtractor()
   {
      return m_extractor;
   }

   void deserialize(void* data, std::size_t byteSize)
   {
      m_extractor.deserialize(static_cast<char*>(data), byteSize);
   }

   template <typename T>
   BufferExtractor& operator>>(T& value)
   {
      m_extractor >> value;

      return *this;
   }

   template <typename T>
   BufferExtractor& operator>>(std::vector<T>& value)
   {
      deserialize(value, value.size());

      return *this;
   }

   template <typename T>
   void deserialize(std::vector<T>& value, std::size_t size)
   {
      m_extractor.deserialize(value, size);
   }

   void assertEmpty()
   {
      m_extractor.assertEmpty();
   }

   const std::byte* getBuffer()
   {
      return m_reader.getBuffer();
   }

private:
   BufferSerializationReader m_reader;
   TypedExtractor            m_extractor;
};

template <typename T>
struct Serializer
{
   /*
    * Serialization interface
    */

   static std::size_t computeSerializedSize(const T& t);

   static void serialize(const T& t, TypedInsertor& insertor);

   static T deserialize(TypedExtractor& extractor);
};

} // namespace ftags::util

#endif // SERIALIZATION_H_INCLUDED
