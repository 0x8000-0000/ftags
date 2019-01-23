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

   const auto resultForInvalidValue = im.getValues(42);

   ASSERT_TRUE(resultForInvalidValue.first == resultForInvalidValue.second);
}

TEST(IndexMapTest, AddOneValueThenGetItBack)
{
   ftags::IndexMap im;

   im.add(42, 3141);

   auto resultForValidValue = im.getValues(42);

   ASSERT_FALSE(resultForValidValue.first == resultForValidValue.second);

   ASSERT_EQ(*resultForValidValue.first, 3141);

   resultForValidValue.first ++;

   ASSERT_TRUE(resultForValidValue.first == resultForValidValue.second);
}

TEST(IndexMapTest, AddTwoValuesThenGetThemBack)
{
   ftags::IndexMap im;

   im.add(42, 3141);
   im.add(42, 999);

   auto resultForValidValue = im.getValues(42);

   ASSERT_FALSE(resultForValidValue.first == resultForValidValue.second);

   ASSERT_EQ(*resultForValidValue.first, 3141);

   resultForValidValue.first ++;

   ASSERT_EQ(*resultForValidValue.first, 999);

   resultForValidValue.first ++;

   ASSERT_TRUE(resultForValidValue.first == resultForValidValue.second);
}

TEST(IndexMapTest, AddValuesToDifferentKeys)
{
   ftags::IndexMap im;

   im.add(999, 42);

   im.add(42, 3141);
   im.add(42, 999);

   im.add(10, 552);

   auto resultForValidValue = im.getValues(42);

   ASSERT_FALSE(resultForValidValue.first == resultForValidValue.second);

   ASSERT_EQ(*resultForValidValue.first, 3141);

   resultForValidValue.first ++;

   ASSERT_EQ(*resultForValidValue.first, 999);

   resultForValidValue.first ++;

   ASSERT_TRUE(resultForValidValue.first == resultForValidValue.second);
}

TEST(IndexMapTest, CanExtendLastElement)
{
   ftags::IndexMap im;

   im.add(999, 555);

   im.add(10, 552);

   for (uint32_t ii = 0; ii < 16; ii ++)
   {
      im.add(42, ii);
   }

   auto resultForValidValue = im.getValues(42);

   ASSERT_EQ(std::distance(resultForValidValue.first, resultForValidValue.second), 16);
}

TEST(IndexMapTest, CanExtendOtherThanLastElement)
{
   ftags::IndexMap im;

   im.add(999, 555);

   im.add(42, 100);

   im.add(10, 552);

   for (uint32_t ii = 0; ii < 16; ii ++)
   {
      im.add(42, ii);
   }

   auto resultForValidValue = im.getValues(42);

   ASSERT_EQ(std::distance(resultForValidValue.first, resultForValidValue.second), 17);
}

TEST(IndexMapTest, ExtendIntoFreedSpace)
{
   ftags::IndexMap im;

   im.add(999, 555);

   im.add(42, 100);

   im.add(10, 777);

   for (uint32_t ii = 0; ii < 3; ii ++)
   {
      im.add(42, ii);
   }

   im.removeKey(10);

   for (uint32_t ii = 10; ii < 20; ii ++)
   {
      im.add(42, ii);
   }

   auto resultForValidValue = im.getValues(42);

   ASSERT_EQ(std::distance(resultForValidValue.first, resultForValidValue.second), 14);
}

TEST(IndexMapTest, AllocateThenDeleteThreeBlocks)
{
   ftags::IndexMap im;

   for (uint32_t ii = 0; ii < 15; ii ++)
   {
      im.add(10, ii);
   }

   for (uint32_t ii = 0; ii < 9; ii ++)
   {
      im.add(20, ii);
   }

   for (uint32_t ii = 0; ii < 4; ii ++)
   {
      im.add(30, ii);
   }

   for (uint32_t ii = 0; ii < 8; ii ++)
   {
      im.add(40, ii);
   }

   im.removeKey(10);
   im.removeKey(30);
   im.removeKey(20);

   for (uint32_t ii = 0; ii < 4; ii ++)
   {
      im.add(50, ii);
   }
}
