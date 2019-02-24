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

#include <tags.h>
#include <tags_builder.h>

#include <gtest/gtest.h>

#include <vector>

TEST(TagsIndexTest, IndexOneFile)
{
   std::vector<const char*> arguments;
   arguments.push_back("-Wall");
   arguments.push_back("-Wextra");
   ftags::Tags tagsDb = ftags::parseTranslationUnit("hello.cc", arguments);

   std::vector<ftags::Record*> functions = tagsDb.getFunctions();

   ASSERT_EQ(1, functions.size());
}

