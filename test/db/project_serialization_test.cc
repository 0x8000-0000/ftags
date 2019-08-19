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

#include <project.h>
#include <serialization.h>

#include <gtest/gtest.h>

#include <filesystem>

TEST(ProjectSerializationTest, RecordVector)
{
   std::vector<ftags::Record> input;

   ftags::Record one   = {};
   ftags::Record two   = {};
   ftags::Record three = {};

   one.symbolNameKey   = 1;
   two.symbolNameKey   = 2;
   three.symbolNameKey = 3;

   input.push_back(one);
   input.push_back(two);
   input.push_back(three);

   using Serializer = ftags::Serializer<std::vector<ftags::Record>>;

   const size_t           inputSerializedSize = Serializer::computeSerializedSize(input);
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};

   Serializer::serialize(input, insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor           extractor{buffer};
   const std::vector<ftags::Record> output = Serializer::deserialize(extractor);
   extractor.assertEmpty();

   ASSERT_EQ(1, output[0].symbolNameKey);
   ASSERT_EQ(2, output[1].symbolNameKey);
   ASSERT_EQ(3, output[2].symbolNameKey);
}

TEST(ProjectSerializationTest, CursorSet)
{
   std::vector<const ftags::Record*> input;

   ftags::Record one   = {};
   ftags::Record two   = {};
   ftags::Record three = {};

   ftags::StringTable symbolTable;
   ftags::StringTable fileNameTable;

   one.symbolNameKey   = symbolTable.addKey("alpha");
   two.symbolNameKey   = symbolTable.addKey("beta");
   three.symbolNameKey = symbolTable.addKey("gamma");

   const auto fileKey = fileNameTable.addKey("hello.cc");
   one.location.fileNameKey   = fileKey;
   two.location.fileNameKey   = fileKey;
   three.location.fileNameKey = fileKey;

   input.push_back(&one);
   input.push_back(&two);
   input.push_back(&three);

   ftags::CursorSet inputSet(input, symbolTable, fileNameTable);

   const size_t           inputSerializedSize = inputSet.computeSerializedSize();
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

TEST(ProjectSerializationTest, DeserializedProjectDbEqualsInput)
{
   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

   const auto path = std::filesystem::current_path();

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-isystem",
      "/usr/include",
   };

   {
      const auto libPath = path / "test" / "db" / "data" / "multi-module" / "lib.cc";
      ASSERT_TRUE(std::filesystem::exists(libPath));

      ftags::StringTable           symbolTable;
      ftags::StringTable           fileNameTable;
      const ftags::TranslationUnit libCpp =
         ftags::TranslationUnit::parse(libPath, arguments, symbolTable, fileNameTable);

      tagsDb.addTranslationUnit(libPath, libCpp, symbolTable, fileNameTable);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));

      ftags::StringTable           symbolTable;
      ftags::StringTable           fileNameTable;
      const ftags::TranslationUnit testCpp =
         ftags::TranslationUnit::parse(testPath, arguments, symbolTable, fileNameTable);

      tagsDb.addTranslationUnit(testPath, testCpp, symbolTable, fileNameTable);
   }

   std::vector<std::byte> buffer(/* size = */ tagsDb.computeSerializedSize());

   ftags::BufferInsertor insertor{buffer};

   tagsDb.serialize(insertor);

   ftags::BufferExtractor extractor{buffer};
   ftags::ProjectDb       restoredTagsDb = ftags::ProjectDb::deserialize(extractor);

   ASSERT_EQ(tagsDb, restoredTagsDb);
}

TEST(ProjectSerializationTest, FindVariablesInDeserializedProjectDb)
{
   std::vector<std::byte> buffer;

   {
      ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

      const auto path = std::filesystem::current_path();

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
         "-isystem",
         "/usr/include",
      };

      {
         const auto libPath = path / "test" / "db" / "data" / "multi-module" / "lib.cc";
         ASSERT_TRUE(std::filesystem::exists(libPath));

         ftags::StringTable           symbolTable;
         ftags::StringTable           fileNameTable;
         const ftags::TranslationUnit libCpp =
            ftags::TranslationUnit::parse(libPath, arguments, symbolTable, fileNameTable);

         tagsDb.addTranslationUnit(libPath, libCpp, symbolTable, fileNameTable);
      }

      {
         const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
         ASSERT_TRUE(std::filesystem::exists(testPath));

         ftags::StringTable           symbolTable;
         ftags::StringTable           fileNameTable;
         const ftags::TranslationUnit testCpp =
            ftags::TranslationUnit::parse(testPath, arguments, symbolTable, fileNameTable);

         tagsDb.addTranslationUnit(testPath, testCpp, symbolTable, fileNameTable);
      }

      buffer.resize(tagsDb.computeSerializedSize());

      ftags::BufferInsertor insertor{buffer};

      tagsDb.serialize(insertor);

      {
         const std::vector<const ftags::Record*> countDefinition = tagsDb.findDefinition("count");
         ASSERT_EQ(1, countDefinition.size());

         const std::vector<const ftags::Record*> countReference = tagsDb.findReference("count");
         ASSERT_EQ(1, countReference.size());

         const std::vector<const ftags::Record*> allCount = tagsDb.findSymbol("count");
         ASSERT_EQ(2, allCount.size());

         const std::vector<const ftags::Record*> argReference = tagsDb.findReference("arg");
         ASSERT_EQ(3, argReference.size());

         const std::vector<const ftags::Record*> allArg = tagsDb.findSymbol("arg");
         ASSERT_EQ(6, allArg.size());
      }
   }

   ftags::BufferExtractor extractor{buffer};
   ftags::ProjectDb       restoredTagsDb = ftags::ProjectDb::deserialize(extractor);

   std::string restoredName = restoredTagsDb.getName();
   ASSERT_STREQ("test", restoredName.data());

   {
      const std::vector<const ftags::Record*> countDefinition = restoredTagsDb.findDefinition("count");
      ASSERT_EQ(1, countDefinition.size());

      const std::vector<const ftags::Record*> countReference = restoredTagsDb.findReference("count");
      ASSERT_EQ(1, countReference.size());

      const std::vector<const ftags::Record*> allCount = restoredTagsDb.findSymbol("count");
      ASSERT_EQ(2, allCount.size());

      const std::vector<const ftags::Record*> argReference = restoredTagsDb.findReference("arg");
      ASSERT_EQ(3, argReference.size());

      const std::vector<const ftags::Record*> allArg = restoredTagsDb.findSymbol("arg");
      ASSERT_EQ(6, allArg.size());
   }
}
