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

#ifndef SERIALIZATION_LEGACY_H_INCLUDED
#define SERIALIZATION_LEGACY_H_INCLUDED

#include <serialization.h>

namespace ftags::util
{

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

} // namespace ftags::util

#endif // SERIALIZATION_LEGACY_H_INCLUDED
