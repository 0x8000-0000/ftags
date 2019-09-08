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

#ifndef SERIALIZATION_IOSTREAM_H_INCLUDED
#define SERIALIZATION_IOSTREAM_H_INCLUDED

#include <serialization.h>

#include <fstream>
#include <string_view>

namespace ftags::util
{

class OfstreamSerializationWriter : public SerializationWriter
{
public:
   OfstreamSerializationWriter(std::string_view fileName, std::size_t size) :
      m_stream{fileName.data(), std::ios::binary}, m_size{size}
   {
      if (m_size == 0)
      {
         throw(std::logic_error("Invalid stream size"));
      }
   }

   void serialize(const char* data, std::size_t byteSize) override
   {
      assert(byteSize <= m_size);
      m_size -= byteSize;
      m_stream.write(data, static_cast<std::streamsize>(byteSize));
   }

#ifndef NDEBUG
   void assertEmpty() override
   {
      assert(m_size == 0);
   }
#endif

private:
   std::ofstream m_stream;
   std::size_t   m_size;
};

class IfstreamSerializationReader : public SerializationReader
{
public:
   IfstreamSerializationReader(std::string_view fileName, std::size_t size) :
      m_stream{fileName.data(), std::ios::binary}, m_size{size}
   {
      if (m_size == 0)
      {
         throw(std::logic_error("Invalid stream size"));
      }
   }

   void deserialize(char* data, std::size_t byteSize) override
   {
      assert(byteSize <= m_size);
      m_size -= byteSize;
      m_stream.read(data, static_cast<std::streamsize>(byteSize));
   }

#ifndef NDEBUG
   void assertEmpty() override
   {
      assert(m_size == 0);
   }
#endif

private:
   std::ifstream m_stream;
   std::size_t   m_size;
};
} // namespace ftags::util

#endif // SERIALIZATION_IOSTREAM_H_INCLUDED
