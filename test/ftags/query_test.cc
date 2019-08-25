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
}
