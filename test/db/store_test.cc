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

#include <algorithm>

/*
 * Intrusive, black-box tests for Store.
 */

using Store = ftags::Store<uint32_t, uint32_t, 24>;

TEST(StoreMapTest, FirstAllocationReturnsKey1)
{
   Store store;

   const auto blockOne = store.allocate(8);

   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
}

TEST(StoreMapTest, BlockIsRecycled)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);

   std::fill_n(blockOne.iterator, blockSize, 1);

   store.deallocate(blockOne.key, blockSize);

   const auto blockTwo = store.allocate(blockSize);

   std::fill_n(blockTwo.iterator, blockSize, 2);

   ASSERT_EQ(blockTwo.key, store.FirstKeyValue);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced1)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   store.deallocate(blockOne.key, blockSize);
   store.deallocate(blockTwo.key, blockSize);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   ASSERT_EQ(blockThree.key, store.FirstKeyValue);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced2)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   store.deallocate(blockOne.key, blockSize);
   store.deallocate(blockTwo.key, blockSize);

   const auto blockThree = store.allocate(2 * blockSize);
   std::fill_n(blockThree.iterator, 2 * blockSize, 3);

   ASSERT_EQ(blockThree.key, store.FirstKeyValue);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced3)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   store.deallocate(blockOne.key, blockSize);
   store.deallocate(blockTwo.key, blockSize);

   const auto blockThree = store.allocate(3 * blockSize);
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);

   ASSERT_EQ(blockThree.key, store.FirstKeyValue);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced4)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   ASSERT_EQ(blockTwo.key, store.FirstKeyValue + blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   const auto blockThree = store.allocate(3 * blockSize);
   ASSERT_EQ(blockThree.key, store.FirstKeyValue + 2 * blockSize);
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);

   store.deallocate(blockTwo.key, blockSize);
   store.deallocate(blockOne.key, blockSize);

   const auto blockFour = store.allocate(2 * blockSize);
   ASSERT_EQ(blockFour.key, store.FirstKeyValue);
   std::fill_n(blockFour.iterator, blockSize, 4);
}

TEST(StoreMapTest, ExtendAllocatedBlock1)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, (1 << 24) - store.FirstKeyValue - blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 2);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock2)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);
   store.deallocate(blockTwo.key, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, (1 << 24) - store.FirstKeyValue - blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 2);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock3)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   store.deallocate(blockTwo.key, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 5);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock4)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   store.deallocate(blockTwo.key, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 3 * blockSize);
   std::fill_n(iter, blockSize, 5);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock5)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   ASSERT_EQ(blockTwo.key, store.FirstKeyValue + blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   ASSERT_EQ(blockThree.key, store.FirstKeyValue + 3 * blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   store.deallocate(blockTwo.key, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 9);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);

   const auto blockFour = store.allocate(blockSize);
   ASSERT_EQ(blockFour.key, store.FirstKeyValue + 2 * blockSize);
   std::fill_n(blockFour.iterator, blockSize, 4);
}


TEST(StoreMapTest, SerializeSingleSegment)
{
   Store store;

   const std::size_t blockSize = 8;

   // setup
   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.FirstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   store.deallocate(blockTwo.key, 2 * blockSize);

   // serialize
   const size_t inputSerializedSize = store.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);
   ftags::BufferInsertor insertor{buffer};
   store.serialize(insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor extractor{buffer};
   Store rehydrated = Store::deserialize(extractor);
   extractor.assertEmpty();

   // test
   const auto rehydratedBlockOne = rehydrated.get(blockOne.key);
   const auto valuesCount = std::count(rehydratedBlockOne.first, rehydratedBlockOne.first + blockSize, 1);
   ASSERT_EQ(blockSize, valuesCount);
}


TEST(StoreAllocatorIteratorTest, EmptyStore)
{
   Store store;

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}

TEST(StoreAllocatorIteratorTest, SingleAllocationAtTheBeginning)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);

   std::fill_n(blockOne.iterator, blockSize, 1);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   ASSERT_EQ(blockOne.iterator, allocSeq.iterator);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(nextSeqValid);
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}

TEST(StoreAllocatorIteratorTest, TwoContiguousAllocations)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);
   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize + 2 * blockSize, allocSeq.size);
   ASSERT_EQ(blockOne.iterator, allocSeq.iterator);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(nextSeqValid);
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}

TEST(StoreAllocatorIteratorTest, SingleAllocationAfterGap)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);
   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);
   store.deallocate(blockOne.key, blockSize);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockTwo.key, allocSeq.key);
   ASSERT_EQ(2 * blockSize, allocSeq.size);
   ASSERT_EQ(blockTwo.iterator, allocSeq.iterator);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(nextSeqValid);
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}

TEST(StoreAllocatorIteratorTest, TwoAllocationsWithGap)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);
   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);
   const auto blockThree = store.allocate(3 * blockSize);
   std::fill_n(blockTwo.iterator, 3 * blockSize, 3);
   store.deallocate(blockOne.key, blockSize);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   ASSERT_EQ(blockOne.iterator, allocSeq.iterator);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockThree.key, allocSeq.key);
   ASSERT_EQ(3 * blockSize, allocSeq.size);
   ASSERT_EQ(blockThree.iterator, allocSeq.iterator);

   const bool nextSeqValid2 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid2);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));
}

