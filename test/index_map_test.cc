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

#include <index_map.h>

#include <gtest/gtest.h>

TEST(IndexMapTest, EmptyTableHasNoValues)
{
   ftags::IndexMap im;

   const std::vector<uint32_t> resultForInvalidValue = im.getValues(42);

   ASSERT_TRUE(resultForInvalidValue.empty());
}

TEST(IndexMapTest, AddOneValueThenGetItBack)
{
   ftags::IndexMap im;

   im.add(42, 3141);

   const std::vector<uint32_t> resultForInvalidValue = im.getValues(42);

   ASSERT_FALSE(resultForInvalidValue.empty());

   ASSERT_EQ(resultForInvalidValue.size(), 1);

   ASSERT_EQ(resultForInvalidValue[0], 3141);
}
