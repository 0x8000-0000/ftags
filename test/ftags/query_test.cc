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

#include <query.h>

#include <gtest/gtest.h>

TEST(QueryTest, FindImplicitSymbol)
{
   ftags::query::Query query = ftags::query::Query::parse("find main");

   ASSERT_EQ("main", query.symbolName);
}

TEST(QueryTest, FindExplicitSymbol)
{
   ftags::query::Query query = ftags::query::Query::parse("find symbol main");

   ASSERT_EQ("main", query.symbolName);
}

TEST(QueryTest, FindFunction)
{
   ftags::query::Query query = ftags::query::Query::parse("find function main");

   ASSERT_EQ("main", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Function);
   ASSERT_TRUE(query.nameSpace.empty());
}

TEST(QueryTest, FindAttribute)
{
   ftags::query::Query query = ftags::query::Query::parse("find attribute m_size");

   ASSERT_EQ("m_size", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Attribute);
}

TEST(QueryTest, FindMethod)
{
   ftags::query::Query query = ftags::query::Query::parse("find method size");

   ASSERT_EQ("size", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Method);
}

TEST(QueryTest, FindFunctionInNamespace)
{
   ftags::query::Query query = ftags::query::Query::parse("find function test2::main");

   ASSERT_EQ("main", query.symbolName);
   ASSERT_EQ(query.verb, ftags::query::Query::Verb::Find);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Function);
   ASSERT_FALSE(query.inGlobalNamespace);
   ASSERT_EQ(1, query.nameSpace.size());
}

TEST(QueryTest, FindFunctionInGlobalNamespace)
{
   ftags::query::Query query = ftags::query::Query::parse("find function ::check");

   ASSERT_EQ("check", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Function);
   ASSERT_EQ(query.qualifier, ftags::query::Query::Qualifier::Any);
   ASSERT_TRUE(query.inGlobalNamespace);
   ASSERT_TRUE(query.nameSpace.empty());
}

TEST(QueryTest, IdentifySymbolInRelativePath)
{
   ftags::query::Query query = ftags::query::Query::parse("identify symbol at file.c:12:32");

   ASSERT_EQ(query.verb, ftags::query::Query::Verb::Identify);
   ASSERT_EQ("file.c", query.filePath);
   ASSERT_EQ(12, query.lineNumber);
   ASSERT_EQ(32, query.columnNumber);
}

TEST(QueryTest, IdentifySymbolInPathRelativeToThis)
{
   ftags::query::Query query = ftags::query::Query::parse("identify symbol at ../file.c:12:32");

   ASSERT_EQ(query.verb, ftags::query::Query::Verb::Identify);
   ASSERT_EQ("../file.c", query.filePath);
   ASSERT_EQ(12, query.lineNumber);
   ASSERT_EQ(32, query.columnNumber);
}

TEST(QueryTest, IdentifySymbolInAbsolutePath)
{
   ftags::query::Query query = ftags::query::Query::parse("identify symbol at /path/to/file.c:12:32");

   ASSERT_EQ(query.verb, ftags::query::Query::Verb::Identify);
   ASSERT_EQ("/path/to/file.c", query.filePath);
   ASSERT_EQ(12, query.lineNumber);
   ASSERT_EQ(32, query.columnNumber);
}

TEST(QueryTest, FindOverrideFor)
{
   ftags::query::Query query = ftags::query::Query::parse("find override of foo::Test::check");

   ASSERT_EQ("check", query.symbolName);
   ASSERT_EQ(query.verb, ftags::query::Query::Verb::Find);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Override);
   ASSERT_FALSE(query.inGlobalNamespace);
   ASSERT_EQ(2, query.nameSpace.size());
}

TEST(QueryTest, FindReferencesToFunctionInGlobalNamespace)
{
   ftags::query::Query query = ftags::query::Query::parse("find function reference ::check");

   ASSERT_EQ("check", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Function);
   ASSERT_EQ(query.qualifier, ftags::query::Query::Qualifier::Reference);
   ASSERT_TRUE(query.inGlobalNamespace);
   ASSERT_TRUE(query.nameSpace.empty());
}

TEST(QueryTest, FindDeclarationsOfFunctionInGlobalNamespace)
{
   ftags::query::Query query = ftags::query::Query::parse("find function declaration ::check");

   ASSERT_EQ("check", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Function);
   ASSERT_EQ(query.qualifier, ftags::query::Query::Qualifier::Declaration);
   ASSERT_TRUE(query.inGlobalNamespace);
   ASSERT_TRUE(query.nameSpace.empty());
}

TEST(QueryTest, FindDefinitionsOfFunctionInGlobalNamespace)
{
   ftags::query::Query query = ftags::query::Query::parse("find function definition ::check");

   ASSERT_EQ("check", query.symbolName);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Function);
   ASSERT_EQ(query.qualifier, ftags::query::Query::Qualifier::Definition);
   ASSERT_TRUE(query.inGlobalNamespace);
   ASSERT_TRUE(query.nameSpace.empty());
}

TEST(QueryTest, ListProjects)
{
   ftags::query::Query query = ftags::query::Query::parse("list projects");

   ASSERT_EQ(query.verb, ftags::query::Query::Verb::List);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Project);
}

TEST(QueryTest, ListDependenciesOfTranslationUnit)
{
   ftags::query::Query query = ftags::query::Query::parse("list dependencies of path/to/file.c");

   ASSERT_EQ(query.verb, ftags::query::Query::Verb::List);
   ASSERT_EQ(query.type, ftags::query::Query::Type::Dependency);
   ASSERT_EQ("path/to/file.c", query.filePath);
}
