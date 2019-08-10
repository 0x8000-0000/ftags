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
#include <project.h>

#include <gtest/gtest.h>

TEST(ProjectSerializationTest, RecordVector)
{
   std::vector<ftags::Record> input;

   ftags::Record one = {};
   ftags::Record two = {};
   ftags::Record three = {};

   one.symbolNameKey = 1;
   two.symbolNameKey = 2;
   three.symbolNameKey = 3;

   input.push_back(one);
   input.push_back(two);
   input.push_back(three);

   using Serializer = ftags::Serializer<std::vector<ftags::Record>>;

   const size_t inputSerializedSize = Serializer::computeSerializedSize(input);
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};

   Serializer::serialize(input, insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor extractor{buffer};
   const std::vector<ftags::Record> output = Serializer::deserialize(extractor);
   extractor.assertEmpty();

   ASSERT_EQ(1, output[0].symbolNameKey);
   ASSERT_EQ(2, output[1].symbolNameKey);
   ASSERT_EQ(3, output[2].symbolNameKey);
}

TEST(ProjectSerializationTest, CursorSet)
{
   std::vector<const ftags::Record*> input;

   ftags::Record one = {};
   ftags::Record two = {};
   ftags::Record three = {};

   ftags::StringTable symbolTable;
   ftags::StringTable fileNameTable;

   one.symbolNameKey = symbolTable.addKey("alpha");
   two.symbolNameKey = symbolTable.addKey("beta");
   three.symbolNameKey = symbolTable.addKey("gamma");

   const auto fileKey = fileNameTable.addKey("hello.cc");
   one.fileNameKey = fileKey;
   two.fileNameKey = fileKey;
   three.fileNameKey = fileKey;

   input.push_back(&one);
   input.push_back(&two);
   input.push_back(&three);

   ftags::CursorSet inputSet(input, symbolTable, fileNameTable);

   const size_t inputSerializedSize = inputSet.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};

   inputSet.serialize(insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor extractor{buffer};
   const ftags::CursorSet output = ftags::CursorSet::deserialize(extractor);
   extractor.assertEmpty();

   auto iter = output.begin();
   ASSERT_NE(output.end(), iter);

   const ftags::Cursor outputOne = output.inflateRecord(*iter);

   ASSERT_STREQ("alpha", outputOne.symbolName);
}
