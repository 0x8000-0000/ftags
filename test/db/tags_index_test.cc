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
   auto path = std::filesystem::current_path();

   auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit helloCpp = ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

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
   auto path = std::filesystem::current_path();

   auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit helloCpp = ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

   ASSERT_TRUE(tagsDb.isFileIndexed(helloPath));

   std::vector<const ftags::Record*> functions = tagsDb.getFunctions();
   ASSERT_LT(1, functions.size());
}

TEST(TagsIndexTest, HelloWorldHasMainFunction)
{
   auto path = std::filesystem::current_path();

   auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit helloCpp = ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

   std::vector<const ftags::Record*> results = tagsDb.findDefinition("main");
   ASSERT_EQ(1, results.size());
}

TEST(TagsIndexTest, HelloWorldCallsPrintfFunction)
{
   auto path = std::filesystem::current_path();

   auto helloPath = path / "test" / "db" / "data" / "hello" / "hello.cc";
   ASSERT_TRUE(std::filesystem::exists(helloPath));

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");
   arguments.push_back("-stdlib=libstdc++");
   arguments.push_back("--gcc-toolchain=/usr");

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit helloCpp = ftags::TranslationUnit::parse(helloPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(helloPath, helloCpp);

   std::stringstream output;

   tagsDb.dumpRecords(output);

   std::string result = output.str();
   ASSERT_FALSE(result.empty());

   std::vector<const ftags::Record*> results = tagsDb.findReference("printf");
   ASSERT_EQ(1, results.size());
}

TEST(TagsIndexTest, DistinguishDeclarationFromDefinition)
{
   auto path = std::filesystem::current_path();

   auto translationUnitPath = path / "test" / "db" / "data" / "functions" / "alpha-beta.cc";
   ASSERT_TRUE(std::filesystem::exists(translationUnitPath));

   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");
   arguments.push_back("-isystem");
   arguments.push_back("/usr/include");

   ftags::StringTable     symbolTable;
   ftags::StringTable     fileNameTable;
   ftags::TranslationUnit translationUnit =
      ftags::TranslationUnit::parse(translationUnitPath, arguments, symbolTable, fileNameTable);

   ftags::ProjectDb tagsDb;
   tagsDb.addTranslationUnit(translationUnitPath, translationUnit);

   std::vector<const ftags::Record*> alphaDefinition = tagsDb.findDefinition("alpha");
   ASSERT_EQ(1, alphaDefinition.size());

   std::vector<const ftags::Record*> alphaDeclaration = tagsDb.findDeclaration("alpha");
   ASSERT_EQ(1, alphaDeclaration.size());

   std::vector<const ftags::Record*> betaDefinition = tagsDb.findDefinition("beta");
   ASSERT_EQ(1, betaDefinition.size());

   std::vector<const ftags::Record*> betaDeclaration = tagsDb.findDeclaration("beta");
   ASSERT_EQ(3, betaDeclaration.size());

   std::vector<const ftags::Record*> betaReferences = tagsDb.findReference("beta");
   ASSERT_EQ(2, betaReferences.size());

   std::set<ftags::SymbolType> expectedTypes{ftags::SymbolType::FunctionCallExpression,
                                             ftags::SymbolType::DeclarationReferenceExpression};

   std::set<ftags::SymbolType> actualTypes{betaReferences[0]->getType(), betaReferences[1]->getType()};

   ASSERT_EQ(expectedTypes, actualTypes);
}
