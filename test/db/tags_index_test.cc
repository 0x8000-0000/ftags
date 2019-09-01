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

TEST(TagsIndexTest, IndexOneFile)
{
   /*
    * This assumes we run the test from the root of the build directory.
    * There's no portable way yet to get the path of the current running
    * binary.
    */
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
   tagsDb.parseOneFile(helloPath, arguments);

   ASSERT_TRUE(tagsDb.isFileIndexed(helloPath));
}

TEST(TagsIndexTest, IndexOneFileHasFunctions)
{
   /*
    * This assumes we run the test from the root of the build directory.
    * There's no portable way yet to get the path of the current running
    * binary.
    */
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
   tagsDb.parseOneFile(helloPath, arguments);

   ASSERT_TRUE(tagsDb.isFileIndexed(helloPath));

   const std::vector<const ftags::Record*> functions = tagsDb.getFunctions();
   ASSERT_LT(1, functions.size());
}

TEST(TagsIndexTest, HelloWorldHasMainFunction)
{
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
   tagsDb.parseOneFile(helloPath, arguments);

   const std::vector<const ftags::Record*> results = tagsDb.findDefinition("main");
   ASSERT_EQ(1, results.size());
}

TEST(TagsIndexTest, CanDumpRecords)
{
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-stdlib=libstdc++",
      "--gcc-toolchain=/usr",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ path.string()};
   tagsDb.parseOneFile(helloPath, arguments, true);

   std::stringstream output;

   tagsDb.dumpRecords(output, std::filesystem::current_path());

   const std::string result = output.str();
   ASSERT_FALSE(result.empty());
}

TEST(TagsIndexTest, HelloWorldCallsPrintfFunction)
{
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-stdlib=libstdc++",
      "--gcc-toolchain=/usr",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
   tagsDb.parseOneFile(helloPath, arguments);

   const std::vector<const ftags::Record*> results = tagsDb.findReference("printf");
   ASSERT_EQ(1, results.size());

   const ftags::Cursor cursor0 = tagsDb.inflateRecord(results[0]);
   ASSERT_STREQ("printf", cursor0.symbolName);
}

TEST(TagsIndexTest, IndexEverythingHasPrintfDeclaration)
{
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-stdlib=libstdc++",
      "--gcc-toolchain=/usr",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ path.string()};
   tagsDb.parseOneFile(helloPath, arguments);

   const std::vector<const ftags::Record*> printfDeclaration = tagsDb.findDeclaration("printf");
   ASSERT_EQ(1, printfDeclaration.size());

   const std::vector<const ftags::Record*> printfReference = tagsDb.findReference("printf");
   ASSERT_EQ(1, printfReference.size());
}

TEST(TagsIndexTest, IndexProjectOnlyDoesNotHavePrintfDeclaration)
{
   const auto path = std::filesystem::current_path();

   const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-stdlib=libstdc++",
      "--gcc-toolchain=/usr",
   };

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ path.string()};
   tagsDb.parseOneFile(helloPath, arguments, false);

   const std::vector<const ftags::Record*> printfDeclaration = tagsDb.findDeclaration("printf");
   ASSERT_EQ(0, printfDeclaration.size());

   const std::vector<const ftags::Record*> printfReference = tagsDb.findReference("printf");
   ASSERT_EQ(1, printfReference.size());
}

TEST(TagsIndexTest, DistinguishDeclarationFromDefinition)
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

   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
   tagsDb.parseOneFile(translationUnitPath, arguments);

   const std::vector<const ftags::Record*> alphaDefinition = tagsDb.findDefinition("alpha");
   ASSERT_EQ(1, alphaDefinition.size());

   const std::vector<const ftags::Record*> alphaDeclaration = tagsDb.findDeclaration("alpha");
   ASSERT_EQ(1, alphaDeclaration.size());

   const std::vector<const ftags::Record*> betaDefinition = tagsDb.findDefinition("beta");
   ASSERT_EQ(1, betaDefinition.size());

   const std::vector<const ftags::Record*> betaDeclaration = tagsDb.findDeclaration("beta");
   ASSERT_EQ(1, betaDeclaration.size());

   const std::vector<const ftags::Record*> betaReferences = tagsDb.findReference("beta");
   ASSERT_EQ(1, betaReferences.size());

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

TEST(TagsIndexTest, MultiModuleEliminateDuplicates)
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
      tagsDb.parseOneFile(libPath, arguments);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));
      tagsDb.parseOneFile(testPath, arguments);
   }

   const std::vector<const ftags::Record*> mainDefinition = tagsDb.findDefinition("main");
   ASSERT_EQ(1, mainDefinition.size());

   const std::vector<const ftags::Record*> functionDefinition = tagsDb.findDefinition("function");
   ASSERT_EQ(1, functionDefinition.size());

   const std::vector<const ftags::Record*> functionDeclaration = tagsDb.findDeclaration("function");
   ASSERT_EQ(1, functionDeclaration.size());
}

TEST(TagsIndexTest, InflateResults)
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
      tagsDb.parseOneFile(libPath, arguments);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));
      tagsDb.parseOneFile(testPath, arguments);
   }

   const std::vector<const ftags::Record*> functionDeclaration = tagsDb.findDeclaration("function");
   ASSERT_EQ(1, functionDeclaration.size());

   const ftags::CursorSet originalCursorSet = tagsDb.inflateRecords(functionDeclaration);

   auto iter = originalCursorSet.begin();
   ASSERT_NE(iter, originalCursorSet.end());

   const ftags::Cursor cursor = originalCursorSet.inflateRecord(*iter);
   ASSERT_STREQ("function", cursor.symbolName);

   ++iter;
   ASSERT_EQ(iter, originalCursorSet.end());
}

TEST(TagsIndexTest, SerializeDeserializeResults)
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
      tagsDb.parseOneFile(libPath, arguments);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));
      tagsDb.parseOneFile(testPath, arguments);
   }

   const std::vector<const ftags::Record*> functionDeclaration = tagsDb.findDeclaration("function");
   ASSERT_EQ(1, functionDeclaration.size());

   const ftags::CursorSet originalCursorSet = tagsDb.inflateRecords(functionDeclaration);

   const std::size_t      bufferSpace = originalCursorSet.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ bufferSpace);
   BufferInsertor         insertor(buffer);
   originalCursorSet.serialize(insertor);
   BufferExtractor extractor(buffer);

   const ftags::CursorSet restoredCursorSet = ftags::CursorSet::deserialize(extractor);

   auto iter = restoredCursorSet.begin();
   ASSERT_NE(iter, restoredCursorSet.end());

   const ftags::Cursor cursor = restoredCursorSet.inflateRecord(*iter);
   ASSERT_STREQ("function", cursor.symbolName);

   ++iter;
   ASSERT_EQ(iter, restoredCursorSet.end());
}

TEST(TagsIndexTest, FindVariables)
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
      tagsDb.parseOneFile(libPath, arguments);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));
      tagsDb.parseOneFile(testPath, arguments);
   }

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

TEST(TagsIndexTest, MergeProjectDatabases)
{
   ftags::ProjectDb mergedDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

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

      ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
      tagsDb.parseOneFile(libPath, arguments);

      mergedDb.mergeFrom(tagsDb);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));

      ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};
      tagsDb.parseOneFile(testPath, arguments);

      mergedDb.mergeFrom(tagsDb);
   }

   const std::vector<const ftags::Record*> countDefinition = mergedDb.findDefinition("count");
   ASSERT_EQ(1, countDefinition.size());

   const std::vector<const ftags::Record*> countReference = mergedDb.findReference("count");
   ASSERT_EQ(1, countReference.size());

   const std::vector<const ftags::Record*> allCount = mergedDb.findSymbol("count");
   ASSERT_EQ(2, allCount.size());

   const std::vector<const ftags::Record*> argReference = mergedDb.findReference("arg");
   ASSERT_EQ(3, argReference.size());

   const std::vector<const ftags::Record*> allArg = mergedDb.findSymbol("arg");
   ASSERT_EQ(6, allArg.size());
}

TEST(TagsIndexTest, IdentifySymbols)
{
   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

   const auto path = std::filesystem::current_path();

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-isystem",
      "/usr/include",
   };

   const auto libPath = path / "test" / "db" / "data" / "multi-module" / "lib.cc";
   ASSERT_TRUE(std::filesystem::exists(libPath));
   tagsDb.parseOneFile(libPath, arguments);

   const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
   ASSERT_TRUE(std::filesystem::exists(testPath));
   tagsDb.parseOneFile(testPath, arguments);

   const std::vector<const ftags::Record*> line3Records = tagsDb.identifySymbol(libPath.string(), 3, 5);
   ASSERT_EQ(2, line3Records.size());

   const std::vector<const ftags::Record*> line11Records = tagsDb.identifySymbol(libPath.string(), 11, 14);
   ASSERT_EQ(1, line11Records.size());
}

TEST(TagsIndexTest, FindMacroDefinition)
{
   ftags::ProjectDb tagsDb{/* name = */ "test", /* rootDirectory = */ "/tmp"};

   const auto path = std::filesystem::current_path();

   const std::vector<const char*> arguments = {
      "-Wall",
      "-Wextra",
      "-isystem",
      "/usr/include",
   };

   const auto libPath = path / "test" / "db" / "data" / "multi-module" / "lib.cc";
   ASSERT_TRUE(std::filesystem::exists(libPath));
   tagsDb.parseOneFile(libPath, arguments);

   // finds one definition only
   const std::vector<const ftags::Record*> doubleMacro = tagsDb.findSymbol("DOUBLY_SO");
   ASSERT_EQ(1, doubleMacro.size());

   const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
   ASSERT_TRUE(std::filesystem::exists(testPath));
   tagsDb.parseOneFile(testPath, arguments);

   // finds definition and declaration
   const std::vector<const ftags::Record*> doubleMacroAgain = tagsDb.findSymbol("DOUBLY_SO");
   ASSERT_EQ(2, doubleMacroAgain.size());
}
