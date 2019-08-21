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

#include <gtest/gtest.h>

#include <map>
#include <vector>

#include <cstddef>
#include <cstdint>

TEST(SerializationTest, MapUintToUint)
{
   std::map<uint32_t, uint32_t> input;
   input[4]  = 42;
   input[0]  = 33;
   input[13] = 2;

   using Serializer = ftags::Serializer<std::map<uint32_t, uint32_t>>;

   const size_t           inputSerializedSize = Serializer::computeSerializedSize(input);
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};
   Serializer::serialize(input, insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor             extractor{buffer};
   const std::map<uint32_t, uint32_t> output = Serializer::deserialize(extractor);
   extractor.assertEmpty();

   auto iter = output.find(4);
   ASSERT_NE(output.end(), iter);
   ASSERT_EQ(42, iter->second);
}

TEST(SerializationTest, CharVector)
{
   std::vector<char> input;
   input.push_back('a');
   input.push_back('b');
   input.push_back('c');

   using Serializer = ftags::Serializer<std::vector<char>>;

   const size_t           inputSerializedSize = Serializer::computeSerializedSize(input);
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};

   Serializer::serialize(input, insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor  extractor{buffer};
   const std::vector<char> output = Serializer::deserialize(extractor);
   extractor.assertEmpty();

   ASSERT_EQ(3, output.size());
   ASSERT_EQ('a', output[0]);
   ASSERT_EQ('c', output[2]);
}
