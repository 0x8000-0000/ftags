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
#include <serialization_legacy.h>

#include <gtest/gtest.h>

#include <filesystem>

using ftags::util::BufferExtractor;
using ftags::util::BufferInsertor;
using ftags::util::StringTable;

TEST(ProjectSerializationTestSynthetic, CursorSet)
{
   std::vector<const ftags::Record*> input;

   ftags::Record one   = {};
   ftags::Record two   = {};
   ftags::Record three = {};

   StringTable symbolTable;
   StringTable fileNameTable;

   one.symbolNameKey   = symbolTable.addKey("alpha");
   two.symbolNameKey   = symbolTable.addKey("beta");
   three.symbolNameKey = symbolTable.addKey("gamma");

   const auto fileKey    = fileNameTable.addKey("hello.cc");
   const auto fileDefKey = fileNameTable.addKey("goodbye.cc");

   one.setLocationFileKey(fileKey);
   two.setLocationFileKey(fileKey);
   three.setLocationFileKey(fileKey);

   one.setDefinitionFileKey(fileDefKey);
   two.setDefinitionFileKey(fileDefKey);
   three.setDefinitionFileKey(fileDefKey);

   input.push_back(&one);
   input.push_back(&two);
   input.push_back(&three);

   ftags::CursorSet inputSet(input, symbolTable, fileNameTable);

   const size_t           inputSerializedSize = inputSet.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   BufferInsertor insertor{buffer};

   inputSet.serialize(insertor.getInsertor());
   insertor.assertEmpty();

   BufferExtractor        extractor{buffer};
   const ftags::CursorSet output = ftags::CursorSet::deserialize(extractor.getExtractor());
   extractor.assertEmpty();

   auto iter = output.begin();
   ASSERT_NE(output.end(), iter);

   const ftags::Cursor outputOne = output.inflateRecord(*iter);

   ASSERT_STREQ("alpha", outputOne.symbolName);
}

class ProjectSerializationTest : public ::testing::Test
{
protected:
   static void SetUpTestCase()
   {
      /*
       * This assumes we run the test from the root of the build directory.
       * There's no portable way yet to get the path of the current running
       * binary.
       */
      const auto rootPath = std::filesystem::current_path();

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
      };

      tagsDb = std::make_unique<ftags::ProjectDb>(/* name = */ "multi", /* rootDirectory = */ rootPath.string());

      const auto libPath = rootPath / "test" / "db" / "data" / "multi-module" / "lib.cc";
      ASSERT_TRUE(std::filesystem::exists(libPath));
      tagsDb->parseOneFile(libPath, arguments);

      const auto testPath = rootPath / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));
      tagsDb->parseOneFile(testPath, arguments);
   }

   static void TearDownTestCase()
   {
      tagsDb.reset();
   }

   static std::unique_ptr<ftags::ProjectDb> tagsDb;
};

std::unique_ptr<ftags::ProjectDb> ProjectSerializationTest::tagsDb;

TEST_F(ProjectSerializationTest, DeserializedProjectDbEqualsInput)
{
   tagsDb->assertValid();

   std::vector<std::byte> buffer(/* size = */ tagsDb->computeSerializedSize());

   BufferInsertor insertor{buffer};

   tagsDb->serialize(insertor.getInsertor());

   BufferExtractor  extractor{buffer};
   ftags::ProjectDb restoredTagsDb = ftags::ProjectDb::deserialize(extractor.getExtractor());

   ASSERT_EQ(*tagsDb, restoredTagsDb);
}

TEST_F(ProjectSerializationTest, FindVariablesInDeserializedProjectDb)
{
   std::vector<std::byte> buffer;

   {
      buffer.resize(tagsDb->computeSerializedSize());

      BufferInsertor insertor{buffer};

      tagsDb->serialize(insertor.getInsertor());

      {
         const std::vector<const ftags::Record*> countDefinition = tagsDb->findDefinition("count");
         ASSERT_EQ(countDefinition.size(), 1);

         const std::vector<const ftags::Record*> countReference = tagsDb->findReference("count");
         ASSERT_EQ(countReference.size(), 1);

         const std::vector<const ftags::Record*> allCount = tagsDb->findSymbol("count");
         ASSERT_EQ(allCount.size(), 2);

         const std::vector<const ftags::Record*> argReference = tagsDb->findReference("arg");
         ASSERT_EQ(argReference.size(), 6);

         const std::vector<const ftags::Record*> allArg = tagsDb->findSymbol("arg");
         ASSERT_EQ(allArg.size(), 9);
      }
   }

   BufferExtractor  extractor{buffer};
   ftags::ProjectDb restoredTagsDb = ftags::ProjectDb::deserialize(extractor.getExtractor());

   std::string restoredName = restoredTagsDb.getName();
   ASSERT_STREQ(restoredName.data(), "multi");

   {
      const std::vector<const ftags::Record*> countDefinition = restoredTagsDb.findDefinition("count");
      ASSERT_EQ(countDefinition.size(), 1);

      const std::vector<const ftags::Record*> countReference = restoredTagsDb.findReference("count");
      ASSERT_EQ(countReference.size(), 1);

      const std::vector<const ftags::Record*> allCount = restoredTagsDb.findSymbol("count");
      ASSERT_EQ(allCount.size(), 2);

      const std::vector<const ftags::Record*> argReference = restoredTagsDb.findReference("arg");
      ASSERT_EQ(argReference.size(), 6);

      const std::vector<const ftags::Record*> allArg = restoredTagsDb.findSymbol("arg");
      ASSERT_EQ(allArg.size(), 9);
   }
}
