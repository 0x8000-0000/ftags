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

#include <store.h>

#include <gtest/gtest.h>

/*
 * Intrusive, black-box tests for Store.
 */

using Store = ftags::Store<uint32_t, uint32_t, 24>;

TEST(StoreMapTest, FirstAllocationReturnsKey1)
{
   Store store;

   const auto blockOne = store.allocate(8);

   ASSERT_EQ(blockOne.first, 1);
}

TEST(StoreMapTest, BlockIsRecycled)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);

   std::fill_n(blockOne.second, blockSize, 1);

   store.deallocate(blockOne.first, blockSize);

   const auto blockTwo = store.allocate(blockSize);

   std::fill_n(blockTwo.second, blockSize, 2);

   ASSERT_EQ(blockTwo.first, 1);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced1)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.second, blockSize, 2);

   store.deallocate(blockOne.first, blockSize);
   store.deallocate(blockTwo.first, blockSize);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.second, blockSize, 3);

   ASSERT_EQ(blockThree.first, 1);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced2)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.second, blockSize, 2);

   store.deallocate(blockOne.first, blockSize);
   store.deallocate(blockTwo.first, blockSize);

   const auto blockThree = store.allocate(2 * blockSize);
   std::fill_n(blockThree.second, 2 * blockSize, 3);

   ASSERT_EQ(blockThree.first, 1);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced3)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.second, blockSize, 2);

   store.deallocate(blockOne.first, blockSize);
   store.deallocate(blockTwo.first, blockSize);

   const auto blockThree = store.allocate(3 * blockSize);
   std::fill_n(blockThree.second, 3 * blockSize, 3);

   ASSERT_EQ(blockThree.first, 1);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced4)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.first, 1);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   ASSERT_EQ(blockTwo.first, 1 + blockSize);
   std::fill_n(blockTwo.second, blockSize, 2);

   const auto blockThree = store.allocate(3 * blockSize);
   ASSERT_EQ(blockThree.first, 1 + 2 * blockSize);
   std::fill_n(blockThree.second, 3 * blockSize, 3);

   store.deallocate(blockTwo.first, blockSize);
   store.deallocate(blockOne.first, blockSize);

   const auto blockFour = store.allocate(2 * blockSize);
   ASSERT_EQ(blockFour.first, 1);
   std::fill_n(blockFour.second, blockSize, 4);
}

TEST(StoreMapTest, ExtendAllocatedBlock1)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.first, 1);
   std::fill_n(blockOne.second, blockSize, 1);

   const std::size_t availableSize = store.availableAfter(blockOne.first, blockSize);
   ASSERT_EQ(availableSize, (1 << 24) - 1 - blockSize);

   auto iter = store.extend(blockOne.first, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 2);

   ASSERT_EQ(std::distance(blockOne.second, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock2)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.first, 1);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.second, 2 * blockSize, 2);
   store.deallocate(blockTwo.first, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.first, blockSize);
   ASSERT_EQ(availableSize, (1 << 24) - 1 - blockSize);

   auto iter = store.extend(blockOne.first, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 2);

   ASSERT_EQ(std::distance(blockOne.second, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock3)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.first, 1);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.second, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.second, blockSize, 3);

   store.deallocate(blockTwo.first, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.first, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.first, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 5);

   ASSERT_EQ(std::distance(blockOne.second, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock4)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.first, 1);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.second, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.second, blockSize, 3);

   store.deallocate(blockTwo.first, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.first, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.first, blockSize, 3 * blockSize);
   std::fill_n(iter, blockSize, 5);

   ASSERT_EQ(std::distance(blockOne.second, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock5)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.first, 1);
   std::fill_n(blockOne.second, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   ASSERT_EQ(blockTwo.first, 1 + blockSize);
   std::fill_n(blockTwo.second, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   ASSERT_EQ(blockThree.first, 1 + 3 * blockSize);
   std::fill_n(blockThree.second, blockSize, 3);

   store.deallocate(blockTwo.first, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.first, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.first, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 9);

   ASSERT_EQ(std::distance(blockOne.second, iter), blockSize);

   const auto blockFour = store.allocate(blockSize);
   ASSERT_EQ(blockFour.first, 1 + 2 * blockSize);
   std::fill_n(blockFour.second, blockSize, 4);
}
