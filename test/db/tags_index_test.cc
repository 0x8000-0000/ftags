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

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <vector>

using ftags::util::BufferExtractor;
using ftags::util::BufferInsertor;

class TagsIndexTestHello : public ::testing::Test
{
protected:
   static void SetUpTestCase()
   {
      /*
       * This assumes we run the test from the root of the build directory.
       * There's no portable way yet to get the path of the current running
       * binary.
       */
      auto rootPath = std::filesystem::current_path();
      helloPath     = rootPath / "test" / "db" / "data" / "hello" / "hello.cc";
      ASSERT_TRUE(std::filesystem::exists(helloPath));

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
      };

      tagsDb = std::make_unique<ftags::ProjectDb>(/* name = */ "test", /* rootDirectory = */ rootPath.string());
      tagsDb->parseOneFile(helloPath, arguments, false);
   }

   static void TearDownTestCase()
   {
      tagsDb.reset();
   }

   // static std::filesystem::path rootPath;

   static std::filesystem::path             helloPath;
   static std::unique_ptr<ftags::ProjectDb> tagsDb;
};

std::filesystem::path             TagsIndexTestHello::helloPath;
std::unique_ptr<ftags::ProjectDb> TagsIndexTestHello::tagsDb;

TEST_F(TagsIndexTestHello, IndexOneFile)
{
   ASSERT_TRUE(tagsDb->isFileIndexed(helloPath));
}

TEST_F(TagsIndexTestHello, IndexOneFileHasFunctions)
{
   const std::vector<const ftags::Record*> functions = tagsDb->getFunctions();
   ASSERT_EQ(functions.size(), 1);
}

TEST_F(TagsIndexTestHello, HelloWorldHasMainFunction)
{
   const std::vector<const ftags::Record*> results = tagsDb->findDefinition("main");
   ASSERT_EQ(results.size(), 1);
}

TEST_F(TagsIndexTestHello, CanDumpRecords)
{
   std::stringstream output;

   tagsDb->dumpRecords(output, std::filesystem::current_path());

   const std::string result = output.str();
   ASSERT_FALSE(result.empty());
}

TEST_F(TagsIndexTestHello, HelloWorldCallsPrintfFunction)
{
   const std::vector<const ftags::Record*> results = tagsDb->findReference("printf");
   ASSERT_EQ(results.size(), 1);

   const ftags::Cursor cursor0 = tagsDb->inflateRecord(results[0]);
   ASSERT_STREQ(cursor0.symbolName, "printf");
}

class TagsIndexTestHelloWorld : public ::testing::Test
{
protected:
   static void SetUpTestCase()
   {
      /*
       * This assumes we run the test from the root of the build directory.
       * There's no portable way yet to get the path of the current running
       * binary.
       */
      auto rootPath = std::filesystem::current_path();
      helloPath     = rootPath / "test" / "db" / "data" / "hello" / "hello.cc";
      ASSERT_TRUE(std::filesystem::exists(helloPath));

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
      };

      tagsDb = std::make_unique<ftags::ProjectDb>(/* name = */ "test", /* rootDirectory = */ rootPath.string());
      tagsDb->parseOneFile(helloPath, arguments);
   }

   static void TearDownTestCase()
   {
      tagsDb.reset();
   }

   static std::filesystem::path             helloPath;
   static std::unique_ptr<ftags::ProjectDb> tagsDb;
};

std::filesystem::path             TagsIndexTestHelloWorld::helloPath;
std::unique_ptr<ftags::ProjectDb> TagsIndexTestHelloWorld::tagsDb;

TEST_F(TagsIndexTestHelloWorld, IndexEverythingHasPrintfDeclaration)
{
   const std::vector<const ftags::Record*> printfDeclaration = tagsDb->findDeclaration("printf");
   ASSERT_EQ(printfDeclaration.size(), 1);

   const std::vector<const ftags::Record*> printfReference = tagsDb->findReference("printf");
   ASSERT_EQ(printfReference.size(), 1);
}

TEST_F(TagsIndexTestHello, IndexProjectOnlyDoesNotHavePrintfDeclaration)
{
   const std::vector<const ftags::Record*> printfDeclaration = tagsDb->findDeclaration("printf");
   ASSERT_EQ(printfDeclaration.size(), 0);

   const std::vector<const ftags::Record*> printfReference = tagsDb->findReference("printf");
   ASSERT_EQ(printfReference.size(), 1);
}

class TagsIndexTestFunctions : public ::testing::Test
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

      tagsDb = std::make_unique<ftags::ProjectDb>(/* name = */ "functions", /* rootDirectory = */ rootPath.string());

      const auto translationUnitPath = rootPath / "test" / "db" / "data" / "functions" / "alpha-beta.cc";
      ASSERT_TRUE(std::filesystem::exists(translationUnitPath));
      tagsDb->parseOneFile(translationUnitPath, arguments);
   }

   static void TearDownTestCase()
   {
      tagsDb.reset();
   }

   static std::unique_ptr<ftags::ProjectDb> tagsDb;
};

std::unique_ptr<ftags::ProjectDb> TagsIndexTestFunctions::tagsDb;

TEST_F(TagsIndexTestFunctions, DistinguishDeclarationFromDefinition)
{
   const std::vector<const ftags::Record*> alphaDefinition = tagsDb->findDefinition("alpha");
   ASSERT_EQ(alphaDefinition.size(), 1);

   const std::vector<const ftags::Record*> alphaDeclaration = tagsDb->findDeclaration("alpha");
   ASSERT_EQ(alphaDeclaration.size(), 1);

   const std::vector<const ftags::Record*> betaDefinition = tagsDb->findDefinition("beta");
   ASSERT_EQ(betaDefinition.size(), 1);

   const std::vector<const ftags::Record*> betaDeclaration = tagsDb->findDeclaration("beta");
   ASSERT_EQ(betaDeclaration.size(), 1);

   const std::vector<const ftags::Record*> betaReferences = tagsDb->findReference("beta");
   ASSERT_EQ(betaReferences.size(), 1);

   ASSERT_EQ(ftags::SymbolType::FunctionCallExpression, betaReferences[0]->getType());
}

TEST(TagsIndexTest, ManageTwoTranslationUnits)
{
   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

   {
      const auto path = std::filesystem::current_path();

      const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
      ASSERT_TRUE(std::filesystem::exists(helloPath));

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
      };

      tagsDb.parseOneFile(helloPath, arguments);
   }

   {
      const auto path = std::filesystem::current_path();

      const auto translationUnitPath = path / "test" / "db" / "data" / "functions" / "alpha-beta.cc";
      ASSERT_TRUE(std::filesystem::exists(translationUnitPath));

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
         "-isystem",
         "/usr/include",
      };

      tagsDb.parseOneFile(translationUnitPath, arguments);
   }

   const std::vector<const ftags::Record*> mainDefinition = tagsDb.findDefinition("main");
   ASSERT_EQ(1, mainDefinition.size());

   const std::vector<const ftags::Record*> alphaDefinition = tagsDb.findDefinition("alpha");
   ASSERT_EQ(1, alphaDefinition.size());

   const std::vector<const ftags::Record*> alphaDeclaration = tagsDb.findDeclaration("alpha");
   ASSERT_EQ(1, alphaDeclaration.size());
}

class TagsIndexTestMulti : public ::testing::Test
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

      libPath = rootPath / "test" / "db" / "data" / "multi-module" / "lib.cc";
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

   static std::filesystem::path             libPath;
   static std::unique_ptr<ftags::ProjectDb> tagsDb;
};

std::filesystem::path             TagsIndexTestMulti::libPath;
std::unique_ptr<ftags::ProjectDb> TagsIndexTestMulti::tagsDb;

TEST_F(TagsIndexTestMulti, EliminateDuplicates)
{
   const std::vector<const ftags::Record*> mainDefinition = tagsDb->findDefinition("main");
   ASSERT_EQ(mainDefinition.size(), 1);

   const std::vector<const ftags::Record*> functionDefinition = tagsDb->findDefinition("function");
   ASSERT_EQ(functionDefinition.size(), 1);

   const std::vector<const ftags::Record*> functionDeclaration = tagsDb->findDeclaration("function");
   ASSERT_EQ(functionDeclaration.size(), 1);
}

TEST_F(TagsIndexTestMulti, InflateResults)
{
   const std::vector<const ftags::Record*> functionDeclaration = tagsDb->findDeclaration("function");
   ASSERT_EQ(functionDeclaration.size(), 1);

   const ftags::CursorSet originalCursorSet = tagsDb->inflateRecords(functionDeclaration);

   auto iter = originalCursorSet.begin();
   ASSERT_NE(iter, originalCursorSet.end());

   const ftags::Cursor cursor = originalCursorSet.inflateRecord(*iter);
   ASSERT_STREQ("function", cursor.symbolName);

   ++iter;
   ASSERT_EQ(iter, originalCursorSet.end());
}

TEST_F(TagsIndexTestMulti, SerializeDeserializeResults)
{
   const std::vector<const ftags::Record*> functionDeclaration = tagsDb->findDeclaration("function");
   ASSERT_EQ(functionDeclaration.size(), 1);

   const ftags::CursorSet originalCursorSet = tagsDb->inflateRecords(functionDeclaration);

   const std::size_t      bufferSpace = originalCursorSet.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ bufferSpace);
   BufferInsertor         insertor(buffer);
   originalCursorSet.serialize(insertor.getInsertor());
   BufferExtractor extractor(buffer);

   const ftags::CursorSet restoredCursorSet = ftags::CursorSet::deserialize(extractor.getExtractor());

   auto iter = restoredCursorSet.begin();
   ASSERT_NE(iter, restoredCursorSet.end());

   const ftags::Cursor cursor = restoredCursorSet.inflateRecord(*iter);
   ASSERT_STREQ(cursor.symbolName, "function");

   ++iter;
   ASSERT_EQ(iter, restoredCursorSet.end());
}

TEST_F(TagsIndexTestMulti, FindVariables)
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

TEST_F(TagsIndexTestMulti, MergeProjectDatabases)
{
   ftags::ProjectDb mergedDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

   mergedDb.mergeFrom(*tagsDb);
   mergedDb.mergeFrom(*tagsDb);

   const std::vector<const ftags::Record*> countDefinition = mergedDb.findDefinition("count");
   ASSERT_EQ(countDefinition.size(), 1);

   const std::vector<const ftags::Record*> countReference = mergedDb.findReference("count");
   ASSERT_EQ(countReference.size(), 1);

   const std::vector<const ftags::Record*> allCount = mergedDb.findSymbol("count");
   ASSERT_EQ(allCount.size(), 2);

   const std::vector<const ftags::Record*> argReference = mergedDb.findReference("arg");
   ASSERT_EQ(argReference.size(), 6);

   const std::vector<const ftags::Record*> allArg = mergedDb.findSymbol("arg");
   ASSERT_EQ(allArg.size(), 9);
}

TEST_F(TagsIndexTestMulti, IdentifySymbols)
{
   ASSERT_EQ(tagsDb->identifySymbol(libPath.string(), 3, 4).size(), 0);

   const std::vector<const ftags::Record*> line3Records = tagsDb->identifySymbol(libPath.string(), 3, 5);
   ASSERT_EQ(line3Records.size(), 1);

   ASSERT_EQ(tagsDb->identifySymbol(libPath.string(), 3, 6).size(), 1);
   ASSERT_EQ(tagsDb->identifySymbol(libPath.string(), 3, 7).size(), 1);
   ASSERT_EQ(tagsDb->identifySymbol(libPath.string(), 3, 8).size(), 1);
   ASSERT_EQ(tagsDb->identifySymbol(libPath.string(), 3, 9).size(), 1);
   ASSERT_EQ(tagsDb->identifySymbol(libPath.string(), 3, 10).size(), 0);

   const std::vector<const ftags::Record*> line11Records = tagsDb->identifySymbol(libPath.string(), 11, 14);
   ASSERT_EQ(line11Records.size(), 1);
}

TEST_F(TagsIndexTestMulti, FindMacroDefinition)
{
   // finds definition and declaration
   const std::vector<const ftags::Record*> doubleMacroAgain = tagsDb->findSymbol("DOUBLY_SO");
   ASSERT_EQ(doubleMacroAgain.size(), 2);
}
