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

constexpr unsigned k_DefaultStoreSegmentSize = 24U;
constexpr unsigned k_SmallStoreSegmentSize   = 5U;

using Store      = ftags::util::Store<uint32_t, uint32_t, k_DefaultStoreSegmentSize>;
using SmallStore = ftags::util::Store<uint32_t, uint32_t, k_SmallStoreSegmentSize>; // block size up to 32 - 4

using ftags::util::BufferExtractor;
using ftags::util::BufferInsertor;

TEST(StoreMapTest, FirstAllocationReturnsKey1)
{
   Store store;

   const auto blockOne = store.allocate(8);

   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);

   ASSERT_EQ(8, store.countUsedBlocks());
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

   ASSERT_EQ(blockTwo.key, store.k_firstKeyValue);

   ASSERT_EQ(8, store.countUsedBlocks());
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced1)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   ASSERT_EQ(store.countUsedBlocks(), 16);

   store.deallocate(blockOne.key, blockSize);
   store.deallocate(blockTwo.key, blockSize);

   ASSERT_EQ(store.countUsedBlocks(), 0);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   ASSERT_EQ(blockThree.key, store.k_firstKeyValue);

   ASSERT_EQ(store.countUsedBlocks(), 8);
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

   ASSERT_EQ(blockThree.key, store.k_firstKeyValue);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced3)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   store.deallocate(blockOne.key, blockSize);
   store.deallocate(blockTwo.key, blockSize);

   const auto blockThree = store.allocate(3 * blockSize);
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);

   ASSERT_EQ(blockThree.key, store.k_firstKeyValue);
}

TEST(StoreMapTest, DeletedBlocksAreCoalesced4)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(blockSize);
   ASSERT_EQ(blockTwo.key, store.k_firstKeyValue + blockSize);
   std::fill_n(blockTwo.iterator, blockSize, 2);

   const auto blockThree = store.allocate(3 * blockSize);
   ASSERT_EQ(blockThree.key, store.k_firstKeyValue + 2 * blockSize);
   std::fill_n(blockThree.iterator, 3 * blockSize, 3);

   store.deallocate(blockTwo.key, blockSize);
   store.deallocate(blockOne.key, blockSize);

   const auto blockFour = store.allocate(2 * blockSize);
   ASSERT_EQ(blockFour.key, store.k_firstKeyValue);
   std::fill_n(blockFour.iterator, blockSize, 4);
}

TEST(StoreMapTest, ExtendAllocatedBlock1)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, (1 << 24) - store.k_firstKeyValue - blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 2);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock2)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);
   store.deallocate(blockTwo.key, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, (1 << 24) - store.k_firstKeyValue - blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 2);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);
}

TEST(StoreMapTest, ExtendAllocatedBlock3)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
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
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
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
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   ASSERT_EQ(blockTwo.key, store.k_firstKeyValue + blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   ASSERT_EQ(blockThree.key, store.k_firstKeyValue + 3 * blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   store.deallocate(blockTwo.key, 2 * blockSize);

   const std::size_t availableSize = store.availableAfter(blockOne.key, blockSize);
   ASSERT_EQ(availableSize, 2 * blockSize);

   auto iter = store.extend(blockOne.key, blockSize, 2 * blockSize);
   std::fill_n(iter, blockSize, 9);

   ASSERT_EQ(std::distance(blockOne.iterator, iter), blockSize);

   const auto blockFour = store.allocate(blockSize);
   ASSERT_EQ(blockFour.key, store.k_firstKeyValue + 2 * blockSize);
   std::fill_n(blockFour.iterator, blockSize, 4);
}

TEST(StoreMapTest, SerializeSingleSegment)
{
   Store store;

   const std::size_t blockSize = 8;

   // setup
   const auto blockOne = store.allocate(blockSize);
   ASSERT_EQ(blockOne.key, store.k_firstKeyValue);
   std::fill_n(blockOne.iterator, blockSize, 1);

   const auto blockTwo = store.allocate(2 * blockSize);
   std::fill_n(blockTwo.iterator, 2 * blockSize, 2);

   const auto blockThree = store.allocate(blockSize);
   std::fill_n(blockThree.iterator, blockSize, 3);

   store.deallocate(blockTwo.key, 2 * blockSize);

   // serialize
   const size_t           inputSerializedSize = store.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);
   BufferInsertor         insertor{buffer};
   store.serialize(insertor);
   insertor.assertEmpty();

   BufferExtractor extractor{buffer};
   Store           rehydrated = Store::deserialize(extractor);
   extractor.assertEmpty();

   // test
   const auto rehydratedBlockOne = rehydrated.get(blockOne.key);
   const auto valuesCount        = std::count(rehydratedBlockOne.first, rehydratedBlockOne.first + blockSize, 1);
   ASSERT_EQ(blockSize, valuesCount);
}

TEST(StoreAllocatorIteratorTest, EmptyStore)
{
   Store store;

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_FALSE(allocSeq.isValid);
}

TEST(StoreAllocatorIteratorTest, SingleAllocationAtTheBeginning)
{
   Store store;

   const std::size_t blockSize = 8;

   const auto blockOne = store.allocate(blockSize);

   std::fill_n(blockOne.iterator, blockSize, 1);

   Store::AllocatedSequence allocSeq = store.getFirstAllocatedSequence();
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(allocSeq.isValid);
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
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize + 2 * blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(allocSeq.isValid);
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
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockTwo.key, allocSeq.key);
   ASSERT_EQ(2 * blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockTwo.iterator, allocSeqIter);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(allocSeq.isValid);
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
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   const auto alloc2 = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(alloc2.isValid);

   ASSERT_EQ(blockThree.key, alloc2.key);
   ASSERT_EQ(3 * blockSize, alloc2.size);
   const auto allocSeqIter2 = store.get(alloc2.key).first;
   ASSERT_EQ(blockThree.iterator, allocSeqIter2);

   const auto alloc3 = store.getNextAllocatedSequence(alloc2);
   ASSERT_FALSE(alloc3.isValid);
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
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockThree.key, allocSeq.key);
   ASSERT_EQ(3 * blockSize, allocSeq.size);
   const auto allocSeqIter2 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockThree.iterator, allocSeqIter2);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockFive.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter3 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockFive.iterator, allocSeqIter3);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(allocSeq.isValid);
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
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockOne.key, allocSeq.key);
   ASSERT_EQ(blockSize, allocSeq.size);
   const auto allocSeqIter = store.get(allocSeq.key).first;
   ASSERT_EQ(blockOne.iterator, allocSeqIter);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockThree.key, allocSeq.key);
   ASSERT_EQ(3 * blockSize, allocSeq.size);
   const auto allocSeqIter2 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockThree.iterator, allocSeqIter2);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_TRUE(allocSeq.isValid);

   ASSERT_EQ(blockFive.key, allocSeq.key);
   ASSERT_EQ(blockSize + blockSize, allocSeq.size); // blocks five and six are adjacent
   const auto allocSeqIter3 = store.get(allocSeq.key).first;
   ASSERT_EQ(blockFive.iterator, allocSeqIter3);

   allocSeq = store.getNextAllocatedSequence(allocSeq);
   ASSERT_FALSE(allocSeq.isValid);
}

TEST(StoreAllocatorIteratorTest, OneAllocationThatFillsTheFirstBlock)
{
   SmallStore store;

   const auto blockOne = store.allocate(SmallStore::k_maxContiguousAllocation);
   ASSERT_EQ(blockOne.key, SmallStore::k_firstKeyValue); // intrusive

   const SmallStore::AllocatedSequence allocOne = store.getFirstAllocatedSequence();
   ASSERT_TRUE(allocOne.isValid);

   ASSERT_EQ(allocOne.key, blockOne.key);
   ASSERT_EQ(allocOne.size, SmallStore::k_maxContiguousAllocation);

   const auto allocTwo = store.getNextAllocatedSequence(allocOne);
   ASSERT_FALSE(allocTwo.isValid);
}

TEST(StoreAllocatorIteratorTest, TwoAllocationsThatFillTheFirstSegmentAndAHalf)
{
   SmallStore store;

   const auto blockOne = store.allocate(SmallStore::k_maxContiguousAllocation);
   ASSERT_EQ(blockOne.key, SmallStore::k_firstKeyValue); // intrusive

   const auto blockTwo = store.allocate(SmallStore::k_maxContiguousAllocation / 2);

   const SmallStore::AllocatedSequence alloc = store.getFirstAllocatedSequence();
   ASSERT_TRUE(alloc.isValid);
   ASSERT_EQ(alloc.key, blockOne.key);
   ASSERT_EQ(alloc.size, SmallStore::k_maxContiguousAllocation);

   const auto allocTwo = store.getNextAllocatedSequence(alloc);
   ASSERT_TRUE(allocTwo.isValid);
   ASSERT_EQ(allocTwo.key, blockTwo.key);
   ASSERT_EQ(allocTwo.size, SmallStore::k_maxContiguousAllocation / 2);

   const auto allocThree = store.getNextAllocatedSequence(allocTwo);
   ASSERT_FALSE(allocThree.isValid);
   ASSERT_EQ(allocThree.key, 0);
   ASSERT_EQ(allocThree.size, 0);
}

TEST(StoreAllocatorIteratorTest, TwoAllocationsThatFillTheFirstTwoSegments)
{
   SmallStore store;

   const auto blockOne = store.allocate(SmallStore::k_maxContiguousAllocation);
   ASSERT_EQ(blockOne.key, SmallStore::k_firstKeyValue); // intrusive

   const auto blockTwo = store.allocate(SmallStore::k_maxContiguousAllocation);

   const SmallStore::AllocatedSequence alloc = store.getFirstAllocatedSequence();
   ASSERT_TRUE(alloc.isValid);
   ASSERT_EQ(alloc.key, blockOne.key);
   ASSERT_EQ(alloc.size, SmallStore::k_maxContiguousAllocation);

   const auto alloc2 = store.getNextAllocatedSequence(alloc);
   ASSERT_TRUE(alloc2.isValid);
   ASSERT_EQ(alloc2.key, blockTwo.key);
   ASSERT_EQ(alloc2.size, SmallStore::k_maxContiguousAllocation);

   const auto alloc3 = store.getNextAllocatedSequence(alloc2);
   ASSERT_FALSE(alloc3.isValid);
   ASSERT_EQ(alloc3.key, 0);
   ASSERT_EQ(alloc3.size, 0);
}

TEST(StoreAllocatorIteratorTest, ThreeAllocationsThatFillTheFirstThreeSegments)
{
   SmallStore store;

   const auto blockOne = store.allocate(SmallStore::k_maxContiguousAllocation);
   ASSERT_EQ(blockOne.key, SmallStore::k_firstKeyValue); // intrusive

   const auto blockTwo   = store.allocate(SmallStore::k_maxContiguousAllocation);
   const auto blockThree = store.allocate(SmallStore::k_maxContiguousAllocation);

   const SmallStore::AllocatedSequence alloc = store.getFirstAllocatedSequence();
   ASSERT_TRUE(alloc.isValid);
   ASSERT_EQ(alloc.key, blockOne.key);
   ASSERT_EQ(alloc.size, SmallStore::k_maxContiguousAllocation);

   const auto alloc2 = store.getNextAllocatedSequence(alloc);
   ASSERT_TRUE(alloc2.isValid);
   ASSERT_EQ(alloc2.key, blockTwo.key);
   ASSERT_EQ(alloc2.size, SmallStore::k_maxContiguousAllocation);

   const auto alloc3 = store.getNextAllocatedSequence(alloc2);
   ASSERT_TRUE(alloc3.isValid);
   ASSERT_EQ(alloc3.key, blockThree.key);
   ASSERT_EQ(alloc3.size, SmallStore::k_maxContiguousAllocation);

   const auto alloc4 = store.getNextAllocatedSequence(alloc3);
   ASSERT_FALSE(alloc4.isValid);
   ASSERT_EQ(alloc4.key, 0);
   ASSERT_EQ(alloc4.size, 0);
}
