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

   ftags::StringTable           symbolTable;
   ftags::StringTable           fileNameTable;
   const ftags::TranslationUnit helloCpp =
      ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

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

   ftags::StringTable           symbolTable;
   ftags::StringTable           fileNameTable;
   const ftags::TranslationUnit helloCpp =
      ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

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

   ftags::StringTable           symbolTable;
   ftags::StringTable           fileNameTable;
   const ftags::TranslationUnit helloCpp =
      ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

   const std::vector<const ftags::Record*> results = tagsDb.findDefinition("main");
   ASSERT_EQ(1, results.size());
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

   ftags::StringTable           symbolTable;
   ftags::StringTable           fileNameTable;
   const ftags::TranslationUnit helloCpp =
      ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

   std::stringstream output;

   tagsDb.dumpRecords(output);

   const std::string result = output.str();
   ASSERT_FALSE(result.empty());

   const std::vector<const ftags::Record*> results = tagsDb.findReference("printf");
   ASSERT_EQ(2, results.size());

   const ftags::Cursor cursor0 = tagsDb.inflateRecord(results[0]);
   ASSERT_STREQ("printf", cursor0.symbolName);

   const ftags::Cursor cursor1 = tagsDb.inflateRecord(results[0]);
   ASSERT_STREQ("printf", cursor1.symbolName);
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

   ftags::StringTable           symbolTable;
   ftags::StringTable           fileNameTable;
   const ftags::TranslationUnit translationUnit =
      ftags::TranslationUnit::parse(translationUnitPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(translationUnitPath, translationUnit);

   const std::vector<const ftags::Record*> alphaDefinition = tagsDb.findDefinition("alpha");
   ASSERT_EQ(1, alphaDefinition.size());

   const std::vector<const ftags::Record*> alphaDeclaration = tagsDb.findDeclaration("alpha");
   ASSERT_EQ(1, alphaDeclaration.size());

   const std::vector<const ftags::Record*> betaDefinition = tagsDb.findDefinition("beta");
   ASSERT_EQ(1, betaDefinition.size());

   const std::vector<const ftags::Record*> betaDeclaration = tagsDb.findDeclaration("beta");
   ASSERT_EQ(2, betaDeclaration.size());

   const std::vector<const ftags::Record*> betaReferences = tagsDb.findReference("beta");
   ASSERT_EQ(2, betaReferences.size());

   const std::set<ftags::SymbolType> expectedTypes{ftags::SymbolType::FunctionCallExpression,
                                                   ftags::SymbolType::DeclarationReferenceExpression};

   const std::set<ftags::SymbolType> actualTypes{betaReferences[0]->getType(), betaReferences[1]->getType()};

   ASSERT_EQ(expectedTypes, actualTypes);
}

TEST(TagsIndexTest, ManageTwoTranslationUnits)
{
   ftags::ProjectDb tagsDb;

   {
      const auto path = std::filesystem::current_path();

      const auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
      ASSERT_TRUE(std::filesystem::exists(helloPath));

      const std::vector<const char*> arguments = {
         "-Wall",
         "-Wextra",
      };

      ftags::StringTable           symbolTable;
      ftags::StringTable           fileNameTable;
      const ftags::TranslationUnit helloCpp =
         ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

      tagsDb.addTranslationUnit(helloPath, helloCpp);
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

      ftags::StringTable           symbolTable;
      ftags::StringTable           fileNameTable;
      const ftags::TranslationUnit translationUnit =
         ftags::TranslationUnit::parse(translationUnitPath, arguments, symbolTable, fileNameTable);

      tagsDb.addTranslationUnit(translationUnitPath, translationUnit);
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
   ftags::ProjectDb tagsDb;

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

      tagsDb.addTranslationUnit(libPath, libCpp);
   }

   {
      const auto testPath = path / "test" / "db" / "data" / "multi-module" / "test.cc";
      ASSERT_TRUE(std::filesystem::exists(testPath));

      ftags::StringTable           symbolTable;
      ftags::StringTable           fileNameTable;
      const ftags::TranslationUnit testCpp =
         ftags::TranslationUnit::parse(testPath, arguments, symbolTable, fileNameTable);

      tagsDb.addTranslationUnit(testPath, testCpp);
   }

   const std::vector<const ftags::Record*> mainDefinition = tagsDb.findDefinition("main");
   ASSERT_EQ(1, mainDefinition.size());

   const std::vector<const ftags::Record*> functionDefinition = tagsDb.findDefinition("function");
   ASSERT_EQ(1, functionDefinition.size());

   const std::vector<const ftags::Record*> functionDeclaration = tagsDb.findDeclaration("function");
   ASSERT_EQ(1, functionDeclaration.size());
}
