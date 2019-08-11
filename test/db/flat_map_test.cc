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

#include <flat_map.h>

#include <gtest/gtest.h>

TEST(FlatMapTest, PutAValueGetAValue)
{
   ftags::FlatMapAccumulator<int, int> fma(3);

   fma.add(1, 2);
   fma.add(3, 4);
   fma.add(5, 6);

   ftags::FlatMap<int, int> flatMap = fma.getMap();

   auto iter = flatMap.lookup(1);
   ASSERT_NE(flatMap.none(), iter);
   ASSERT_EQ(2, iter->second);
}

TEST(FlatMapTest, MissingValuesNotFound)
{
   ftags::FlatMapAccumulator<int, int> fma(3);

   fma.add(1, 2);
   fma.add(3, 4);
   fma.add(5, 6);

   ftags::FlatMap<int, int> flatMap = fma.getMap();

   auto iter2 = flatMap.lookup(2);
   ASSERT_EQ(flatMap.none(), iter2);
}
