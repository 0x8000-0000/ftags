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
using SmallStore = ftags::Store<uint32_t, uint32_t, 5>;     // block size up to 32 - 4

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
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

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
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

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

   std::fill_n(blockOne.iterator, blockSize, 9);
   store.deallocate(blockOne.key, blockSize);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockTwo.key, allocSeq.key);
   ASSERT_EQ(2 * blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockTwo.iterator, allocSeqIter);

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
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);

   std::fill_n(blockTwo.iterator, 2 * blockSize, 9);
   store.deallocate(blockTwo.key, 2 * blockSize);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockThree.key, allocSeq.key);
   ASSERT_EQ(3 * blockSize, allocSeq.size);
   const auto allocSeqIter2 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockThree.iterator, allocSeqIter2);

   const bool nextSeqValid2 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(nextSeqValid2);
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}

TEST(StoreAllocatorIteratorTest, ThreeAllocationsWithTwoGaps)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);
   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);
   const auto blockThree = store.allocate(3 * blockSize);
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);
   const auto blockFour = store.allocate(blockSize);
   std::fill_n(blockFour.iterator, blockSize, 4);
   const auto blockFive = store.allocate(blockSize);
   std::fill_n(blockFive.iterator, blockSize, 4);

   std::fill_n(blockTwo.iterator, 2 * blockSize, 9);
   store.deallocate(blockTwo.key, 2 * blockSize);
   std::fill_n(blockFour.iterator, blockSize, 9);
   store.deallocate(blockFour.key, blockSize);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockThree.key, allocSeq.key);
   ASSERT_EQ(3 * blockSize, allocSeq.size);
   const auto allocSeqIter2 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockThree.iterator, allocSeqIter2);

   const bool nextSeqValid2 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid2);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockFive.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter3 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockFive.iterator, allocSeqIter3);

   const bool nextSeqValid3 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(nextSeqValid3);
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}

TEST(StoreAllocatorIteratorTest, ThreeAllocationsWithTwoGapsOverflow)
{
   SmallStore store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);
   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);
   const auto blockThree = store.allocate(3 * blockSize);
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);
   const auto blockFour = store.allocate(blockSize);
   std::fill_n(blockFour.iterator, blockSize, 4);
   const auto blockFive = store.allocate(blockSize);
   std::fill_n(blockFive.iterator, blockSize, 5);
   const auto blockSix = store.allocate(blockSize);
   std::fill_n(blockSix.iterator, blockSize, 6);
   // const auto blockSeven = store.allocate(blockSize / 2);
   // std::fill_n(blockSeven.iterator, blockSize, 7);

   std::fill_n(blockTwo.iterator, 2 * blockSize, 9);
   store.deallocate(blockTwo.key, 2 * blockSize);
   std::fill_n(blockFour.iterator, blockSize, 9);
   store.deallocate(blockFour.key, blockSize);

   SmallStore::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   const bool nextSeqValid = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockThree.key, allocSeq.key);
   ASSERT_EQ(3 * blockSize, allocSeq.size);
   const auto allocSeqIter2 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockThree.iterator, allocSeqIter2);

   const bool nextSeqValid2 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(nextSeqValid2);
   ASSERT_TRUE(store.isValidAllocatedSequence(allocSeq));

   ASSERT_EQ(blockFive.key, allocSeq.key);
   ASSERT_EQ(blockSize + blockSize, allocSeq.size);  // blocks five and six are adjacent
   const auto allocSeqIter3 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockFive.iterator, allocSeqIter3);

   const bool nextSeqValid3 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(nextSeqValid3);
   ASSERT_FALSE(store.isValidAllocatedSequence(allocSeq));
}
