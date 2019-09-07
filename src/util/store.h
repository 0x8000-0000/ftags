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

#ifndef STORE_H_INCLUDED
#define STORE_H_INCLUDED

#include <serialization.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include <cassert>

namespace ftags::util
{

/** Allocates blocks of T, returning a cookie
 *
 * The store only guarantees that successive allocations do not overlap. The
 * store does not track the allocations in any way, other than to manage and
 * recycle the free blocks.
 *
 * The cookie acts similarly to a synthetic pointer, but it can be converted
 * to the underlying iterator via an explicit request only.
 */
static constexpr uint32_t k_DefaultStoreSegmentSize = 24U;

template <typename T, typename K, unsigned SegmentSizeBits = k_DefaultStoreSegmentSize>
class Store
{
public:
   using block_size_type = uint32_t;

   static constexpr block_size_type k_maxSegmentSize      = (1U << SegmentSizeBits);
   static constexpr block_size_type k_maxSegmentCount     = (1U << ((sizeof(K) * 8) - SegmentSizeBits));
   static constexpr block_size_type k_OffsetInSegmentMask = (k_maxSegmentSize - 1);
   static constexpr block_size_type k_segmentIndexMask    = ((k_maxSegmentCount - 1) << SegmentSizeBits);

public:
   using iterator       = T*;
   using const_iterator = const T*;

   using key_type = K;
   using Key      = K;

   static constexpr block_size_type k_firstKeyValue           = 4;
   static constexpr block_size_type k_maxContiguousAllocation = (k_maxSegmentSize - k_firstKeyValue);

   struct Allocation
   {
      K  key;
      T* iterator;
   };

   Store()  = default;
   ~Store() = default;

   Store(const Store& other) = delete;
   const Store& operator=(const Store& other) = delete;

   Store(Store&& other) noexcept :
      m_segment{std::move(other.m_segment)},
      m_freeBlocks{std::move(other.m_freeBlocks)},
      m_freeBlocksIndex{std::move(other.m_freeBlocksIndex)}
   {
   }

   Store& operator=(Store&& other) noexcept
   {
      m_segment         = std::move(other.m_segment);
      m_freeBlocks      = std::move(other.m_freeBlocks);
      m_freeBlocksIndex = std::move(other.m_freeBlocksIndex);
      return *this;
   }

   /** Allocates some units of T
    *
    * @param size is the number of units of T
    * @return a pair of key and an iterator to the first allocated unit
    */
   Allocation allocate(block_size_type size);

   /** Construct a non-trivial T instance in place.
    *
    * TODO(signbit) figure out with type traits if we need to call
    * construct/destruct automatically.
    */
   Allocation construct();

   /** Destruct all allocated T instances.
    */
   void destruct();

   /** Requests access to an allocated block by key
    *
    * @param key identifies the block of T
    * @return a pair of an iterator to the first allocated unit and an
    *    iterator to the last accessible unit in the segment
    */
   std::pair<const_iterator, const_iterator> get(K key) const;

   std::pair<iterator, const_iterator> get(K key);

   /** Releases s units of T allocated at key
    *
    * @param key identifies the block of T
    * @param size is the number of units of T
    * @note this method is optional
    */
   void deallocate(K key, block_size_type size);

   /** Return the number of units we know can be allocated starting at key
    *
    * @param key identifies a block
    * @param size is the number of units of T
    *
    * @note This succeeds if a block starting at K is in the free list or
    * if K is right at the end of the current allocation.
    * @see allocateAt
    */
   block_size_type availableAfter(K key, block_size_type size) const noexcept;

   /** Extends the block at key K
    *
    * @param key identifies the block of T
    * @param oldSize is the number of units of T currently allocated
    * @param newSize is the number of units of T desired
    *
    * @note For this operation to succeed, availableAt(K, size) should return
    * more than newSize - size
    * @see availableAt
    */
   iterator extend(K key, block_size_type oldSize, block_size_type newSize);

   /*
    * iteration over contiguously allocated areas
    */
   // TODO (signbit): add begin(), next() and end() for used blocks iterator
   struct AllocatedSequence
   {
      K key;

      block_size_type size : SegmentSizeBits;
      block_size_type isUsed : 1;
      block_size_type isValid : 1;
   };

   static_assert(sizeof(AllocatedSequence) == (2 * sizeof(block_size_type)),
                 "AllocatedSequence fits in two pointers");

   AllocatedSequence getFirstAllocatedSequence() const noexcept;
   AllocatedSequence getNextAllocatedSequence(AllocatedSequence allocatedSequence) const noexcept;

   AllocatedSequence getFirstBlock() const noexcept
   {
      return getFirstBlockInSegment(0);
   }
   AllocatedSequence getNextBlock(AllocatedSequence allocatedSequence) const noexcept;

   template <typename F>
   void forEach(F func)
   {
      forEachAllocatedSequence([&func](Key key, T* instance, block_size_type size) {
         for (block_size_type ii = 0; ii < size; ii++)
         {
            func(key + ii, &instance[ii]);
         }
      });
   }

   template <typename F>
   void forEach(F func) const
   {
      forEachAllocatedSequence([&func](Key key, const T* instance, block_size_type size) {
         for (block_size_type ii = 0; ii < size; ii++)
         {
            func(key + ii, &instance[ii]);
         }
      });
   }

   block_size_type countUsedBlocks() const
   {
      block_size_type count = 0;

      forEachAllocatedSequence([&count](Key /* key */, const T* /* ptr */, block_size_type size) { count += size; });

      return count;
   }

   /** Runs a self-check
    */
   void validateInternalState() const;

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::util::BufferInsertor& insertor) const;

   static Store deserialize(ftags::util::BufferExtractor& extractor);

private:
   template <typename F>
   void forEachAllocatedSequence(F func)
   {
      AllocatedSequence allocation = getFirstAllocatedSequence();

      while (allocation.isValid)
      {
         auto pair = get(allocation.key);
         func(allocation.key, &*pair.first, allocation.size);
         allocation = getNextAllocatedSequence(allocation);
      }
   }

   template <typename F>
   void forEachAllocatedSequence(F func) const
   {
      AllocatedSequence allocation = getFirstAllocatedSequence();

      while (allocation.isValid)
      {
         auto pair = get(allocation.key);
         func(allocation.key, &*pair.first, allocation.size);
         allocation = getNextAllocatedSequence(allocation);
      }
   }

   AllocatedSequence getFirstBlockInSegment(block_size_type segmentIndex) const noexcept;
   AllocatedSequence getFirstBlockFollowing(K key) const noexcept;

   static block_size_type getOffsetInSegment(K key)
   {
      return (key & k_OffsetInSegmentMask);
   }

   static block_size_type getSegmentIndex(K key)
   {
      return (key >> SegmentSizeBits) & (k_maxSegmentCount - 1);
   }

   static K makeKey(block_size_type segmentIndex, block_size_type offsetInSegment)
   {
      assert(segmentIndex < k_maxSegmentCount);
      assert(offsetInSegment < k_maxSegmentSize);

      return static_cast<K>((segmentIndex << SegmentSizeBits) | offsetInSegment);
   }

   void addSegment()
   {
      const block_size_type segmentsInUse{static_cast<block_size_type>(m_segment.size())};

      if ((segmentsInUse + 1) >= k_maxSegmentCount)
      {
         throw std::length_error("Exceeded data structure capacity");
      }

      m_segment.emplace_back(std::unique_ptr<std::byte[]>(new std::byte[sizeof(T) * k_maxSegmentSize])); // NOLINT

      const K key{makeKey(segmentsInUse, k_firstKeyValue)};

      recordFreeBlock(key, k_maxContiguousAllocation);
   }

   void recordFreeBlock(K key, block_size_type size)
   {
      assert(key >= k_firstKeyValue);
      assert(size <= k_maxContiguousAllocation);
      m_freeBlocks.insert({size, key});
      m_freeBlocksIndex.insert({key, size});
   }

   void recycleFreeBlock(typename std::multimap<block_size_type, K>::iterator iter)
   {
      m_freeBlocksIndex.erase(iter->second);
      m_freeBlocks.erase(iter);
   }

   void recycleFreeBlock(typename std::multimap<block_size_type, K>::iterator iter,
                         typename std::map<K, block_size_type>::iterator      iterIndex)
   {
      assert(iterIndex->first == iter->second);
      m_freeBlocksIndex.erase(iterIndex);
      m_freeBlocks.erase(iter);
   }

   std::size_t computeSpaceUsedInLastSegment() const;

   /** Segment of contiguous T, up to (1<<B) elements in size.
    */
   std::vector<std::unique_ptr<std::byte[]>> m_segment; // NOLINT

   /** Maps sizes to block indices
    */
   std::multimap<block_size_type, K> m_freeBlocks;

   /** Maps free block indices to the block size
    */
   std::map<K, block_size_type> m_freeBlocksIndex;

   /** Serialization signature
    */
   static constexpr std::string_view k_serializationSignature{"thooh/eiR4sho1v"};
};

/*
 * implementation details
 */
template <typename T, typename K, unsigned SegmentSizeBits>
std::pair<typename Store<T, K, SegmentSizeBits>::const_iterator,
          typename Store<T, K, SegmentSizeBits>::const_iterator>
Store<T, K, SegmentSizeBits>::get(K key) const
{
   if (key == 0)
   {
      return std::pair<typename Store<T, K, SegmentSizeBits>::const_iterator,
                       typename Store<T, K, SegmentSizeBits>::const_iterator>(nullptr, nullptr);
   }

   const block_size_type segmentIndex{getSegmentIndex(key)};
   const block_size_type offsetInSegment{getOffsetInSegment(key)};

   const auto& segment{m_segment.at(segmentIndex)};
   const T*    iter{static_cast<T*>(static_cast<void*>(segment.get()))};
   const T*    segmentEnd = iter + k_maxSegmentSize;
   iter += offsetInSegment;

   assert(offsetInSegment < k_maxSegmentSize);

   return std::pair<typename Store<T, K, SegmentSizeBits>::const_iterator,
                    typename Store<T, K, SegmentSizeBits>::const_iterator>(iter, segmentEnd);
}

template <typename T, typename K, unsigned SegmentSizeBits>
std::pair<typename Store<T, K, SegmentSizeBits>::iterator, typename Store<T, K, SegmentSizeBits>::const_iterator>
Store<T, K, SegmentSizeBits>::get(K key)
{
   if (key == 0)
   {
      return std::pair<typename Store<T, K, SegmentSizeBits>::iterator,
                       typename Store<T, K, SegmentSizeBits>::const_iterator>(nullptr, nullptr);
   }

   const block_size_type segmentIndex    = getSegmentIndex(key);
   const block_size_type offsetInSegment = getOffsetInSegment(key);

   auto&    segment = m_segment.at(segmentIndex);
   T*       iter{static_cast<T*>(static_cast<void*>(segment.get()))};
   const T* segmentEnd = iter + k_maxSegmentSize;
   iter += offsetInSegment;

   assert(offsetInSegment < k_maxSegmentSize);

   return std::pair<typename Store<T, K, SegmentSizeBits>::iterator,
                    typename Store<T, K, SegmentSizeBits>::const_iterator>(iter, segmentEnd);
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::Allocation Store<T, K, SegmentSizeBits>::allocate(block_size_type size)
{
   if (size >= k_maxSegmentSize)
   {
      throw std::length_error(std::string("Can't store objects that large: ") + std::to_string(size));
   }

   /*
    * check if there is a suitable free block available
    */
   auto blockIter{m_freeBlocks.lower_bound(size)};

   if (blockIter != m_freeBlocks.end())
   {
      const block_size_type availableSize{blockIter->first};
      assert(availableSize >= size);

      const K key{blockIter->second};
      recycleFreeBlock(blockIter);

      const block_size_type segmentIndex{getSegmentIndex(key)};
      const block_size_type offsetInSegment{getOffsetInSegment(key)};

      if (availableSize != size)
      {
         // shrink the leftover block
         const block_size_type leftOverOffsetInSegment{offsetInSegment + size};
         const K               reminderBlockKey{makeKey(segmentIndex, leftOverOffsetInSegment)};
         const block_size_type reminderBlockSize{availableSize - size};
         recordFreeBlock(reminderBlockKey, reminderBlockSize);
      }

      T* iter{static_cast<T*>(static_cast<void*>(m_segment[segmentIndex].get()))};
      iter += offsetInSegment;

      return Allocation{key, iter};
   }

   /*
    * no free block of the requisite size
    */

   addSegment();

   return allocate(size);
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::Allocation Store<T, K, SegmentSizeBits>::construct()
{
   Allocation alloc = allocate(1);

   T* obj = new (&*alloc.iterator) T; // NOLINT
   (void)obj;

   return alloc;
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::destruct()
{
   forEachAllocatedSequence([](Key /* key */, T* instance, block_size_type size) {
      for (block_size_type ii = 0; ii < size; ii++)
      {
         instance[ii].~T();
      }
   });
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::block_size_type
Store<T, K, SegmentSizeBits>::availableAfter(K key, typename Store<T, K, SegmentSizeBits>::block_size_type size) const
   noexcept
{
   if (key == 0)
   {
      return 0;
   }

   const block_size_type segmentIndex{getSegmentIndex(key)};
   const block_size_type offsetInSegment{getOffsetInSegment(key)};

   const K candidateKey{makeKey(segmentIndex, offsetInSegment + size)};

#ifdef FTAGS_STRICT_CHECKING
   auto blockIter{std::find_if(m_freeBlocks.begin(), m_freeBlocks.end(), [candidateKey](const auto& pp) {
      return pp.second == candidateKey;
   })};
#endif

   auto blockIndex = m_freeBlocksIndex.find(candidateKey);
   if (blockIndex != m_freeBlocksIndex.end())
   {
#ifdef FTAGS_STRICT_CHECKING
      assert(blockIter != m_freeBlocks.end());
      assert(blockIter->first == blockIndex->second);
#endif
      return blockIndex->second;
   }

#ifdef FTAGS_STRICT_CHECKING
   assert(blockIter == m_freeBlocks.end());
#endif

   return 0;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::iterator
Store<T, K, SegmentSizeBits>::extend(K key, block_size_type oldSize, block_size_type newSize)
{
   if (key == 0)
   {
      throw std::invalid_argument("Key 0 is invalid");
   }

   if (oldSize == newSize)
   {
      throw std::invalid_argument("Nothing to extend; old size and new size are the same");
   }

   const block_size_type segmentIndex    = getSegmentIndex(key);
   const block_size_type offsetInSegment = getOffsetInSegment(key);

   const K candidateKey{makeKey(segmentIndex, offsetInSegment + oldSize)};

   auto blockIndex = m_freeBlocksIndex.find(candidateKey);
   if (blockIndex == m_freeBlocksIndex.end())
   {
      throw std::logic_error("Can't extend allocation; no free block follows");
   }

   auto range = m_freeBlocks.equal_range(blockIndex->second);

   auto blockIter{
      std::find_if(range.first, range.second, [candidateKey](const auto& pp) { return pp.second == candidateKey; })};

   const block_size_type sizeIncrease{newSize - oldSize};
   const block_size_type availableSize{blockIter->first};
   if (availableSize < sizeIncrease)
   {
      throw std::logic_error("Can't allocate more than what's available");
   }

   recycleFreeBlock(blockIter, blockIndex);

   if (sizeIncrease != availableSize)
   {
      // shrink the leftover block
      const block_size_type leftOverOffsetInSegment{offsetInSegment + newSize};
      const K               reminderBlockKey{makeKey(segmentIndex, leftOverOffsetInSegment)};
      const block_size_type reminderBlockSize{availableSize - sizeIncrease};
      recordFreeBlock(reminderBlockKey, reminderBlockSize);
   }

   auto& segment = m_segment.at(segmentIndex);
   T*    iter{static_cast<T*>(static_cast<void*>(segment.get()))};
   iter += offsetInSegment + oldSize;

   return iter;
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::deallocate(K key, block_size_type size)
{
   const block_size_type segmentIndex{getSegmentIndex(key)};
   const block_size_type offsetInSegment{getOffsetInSegment(key)};

   block_size_type previousBlockKey{0};
   block_size_type previousBlockSize{0};
   block_size_type followingBlockKey{0};
   block_size_type followingBlockSize{0};

   auto previousEraser  = m_freeBlocks.end();
   auto followingEraser = m_freeBlocks.end();

   for (auto groupIter{m_freeBlocks.begin()};
        (groupIter != m_freeBlocks.end()) && (0 == (previousBlockKey * followingBlockKey));
        ++groupIter)
   {
      const block_size_type iterSegmentIndex{getSegmentIndex(groupIter->second)};
      const block_size_type iterOffsetInSegment{getOffsetInSegment(groupIter->second)};
      const block_size_type iterSize{groupIter->first};

      if (segmentIndex == iterSegmentIndex)
      {
         if ((iterOffsetInSegment + iterSize) == offsetInSegment)
         {
            previousBlockKey  = groupIter->second;
            previousBlockSize = iterSize;
            previousEraser    = groupIter;
            continue;
         }

         if ((offsetInSegment + size) == iterOffsetInSegment)
         {
            followingBlockKey  = groupIter->second;
            followingBlockSize = iterSize;
            followingEraser    = groupIter;
         }
      }
   }

   if (previousEraser == followingEraser)
   {
      if (previousEraser != m_freeBlocks.end())
      {
         recycleFreeBlock(previousEraser);
      }
   }
   else
   {
      if (previousEraser != m_freeBlocks.end())
      {
         recycleFreeBlock(previousEraser);
      }

      if (followingEraser != m_freeBlocks.end())
      {
         recycleFreeBlock(followingEraser);
      }
   }

   block_size_type newBlockKey{key};
   block_size_type newBlockSize{size};

   if ((0 == previousBlockKey) && (0 == followingBlockKey))
   {
      // default values - insert block as is
   }
   else if ((0 != previousBlockKey) && (0 != followingBlockKey))
   {
      // adjacent both left and right; credit all size to previous and delete following
      assert(previousBlockKey < key);
      assert(key < followingBlockKey);

      newBlockKey  = previousBlockKey;
      newBlockSize = previousBlockSize + size + followingBlockSize;
   }
   else
   {
      if (0 != previousBlockKey)
      {
         assert(previousBlockKey < key);
         newBlockKey  = previousBlockKey;
         newBlockSize = previousBlockSize + size;
      }
      else
      {
         assert(key < followingBlockKey);
         newBlockSize = size + followingBlockSize;
      }
   }

   recordFreeBlock(newBlockKey, newBlockSize);
}

template <typename T, typename K, unsigned SegmentSizeBits>
std::size_t Store<T, K, SegmentSizeBits>::computeSpaceUsedInLastSegment() const
{
   const auto lastFreeBlockIter = m_freeBlocksIndex.crbegin();
   assert(lastFreeBlockIter != m_freeBlocksIndex.crend());

   const std::size_t sizeOfLastFreeBlock = lastFreeBlockIter->second;

   return k_maxSegmentSize - sizeOfLastFreeBlock;
}

template <typename T, typename K, unsigned SegmentSizeBits>
std::size_t Store<T, K, SegmentSizeBits>::computeSerializedSize() const
{
   const std::size_t segmentCount = m_segment.size();

   std::size_t serializedSize = sizeof(SerializedObjectHeader) + // header
                                sizeof(uint64_t) +               // number of segments
                                sizeof(uint64_t);                // size (used) of last segment

   if (segmentCount != 0U)
   {
      serializedSize = sizeof(SerializedObjectHeader) +                    // header
                       sizeof(uint64_t) +                                  // number of segments
                       sizeof(uint64_t) +                                  // size (used) of last segment
                       (segmentCount - 1) * k_maxSegmentSize * sizeof(T) + // full segments
                       computeSpaceUsedInLastSegment() * sizeof(T) +       // last segment
                       Serializer<std::map<K, block_size_type>>::computeSerializedSize(m_freeBlocksIndex);
   }

   return serializedSize;
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::serialize(ftags::util::BufferInsertor& insertor) const
{
   SerializedObjectHeader header{k_serializationSignature.data()};
   insertor << header;

   const uint64_t segmentCount = m_segment.size();
   insertor << segmentCount;

   if (segmentCount != 0U)
   {
      const uint64_t spaceUsedInLastSegment = computeSpaceUsedInLastSegment();
      insertor << spaceUsedInLastSegment;

      for (uint64_t ii = 0; ii < (segmentCount - 1); ii++)
      {
         insertor.serialize(static_cast<void*>(m_segment[ii].get()), k_maxSegmentSize * sizeof(T));
      }

      insertor.serialize(static_cast<void*>(m_segment[segmentCount - 1].get()), spaceUsedInLastSegment * sizeof(T));

      Serializer<std::map<K, block_size_type>>::serialize(m_freeBlocksIndex, insertor);
   }
   else
   {
      const uint64_t spaceUsedInLastSegment = 0;
      insertor << spaceUsedInLastSegment;
   }
}

template <typename T, typename K, unsigned SegmentSizeBits>
Store<T, K, SegmentSizeBits> Store<T, K, SegmentSizeBits>::deserialize(ftags::util::BufferExtractor& extractor)
{
   Store<T, K, SegmentSizeBits> retval;

   SerializedObjectHeader header;
   extractor >> header;

   assert(memcmp(header.m_objectType, k_serializationSignature.data(), sizeof(header.m_objectType)) == 0);

   uint64_t segmentCount = 0;
   extractor >> segmentCount;

   uint64_t spaceUsedInLastSegment = 0;
   extractor >> spaceUsedInLastSegment;

   if (segmentCount != 0U)
   {
      for (uint64_t ii = 0; ii < (segmentCount - 1); ii++)
      {
         retval.m_segment.emplace_back(
            std::unique_ptr<std::byte[]>(new std::byte[k_maxSegmentSize * sizeof(T)])); // NOLINT
         auto& segment = retval.m_segment.back();
         extractor.deserialize(static_cast<void*>(segment.get()), k_maxSegmentSize * sizeof(T));
      }

      {
         retval.m_segment.emplace_back(
            std::unique_ptr<std::byte[]>(new std::byte[k_maxSegmentSize * sizeof(T)])); // NOLINT
         auto& segment = retval.m_segment.back();
         extractor.deserialize(static_cast<void*>(segment.get()), spaceUsedInLastSegment * sizeof(T));
      }

      assert(retval.m_freeBlocksIndex.size() == 0);
      assert(retval.m_freeBlocks.size() == 0);

      retval.m_freeBlocksIndex = Serializer<std::map<K, block_size_type>>::deserialize(extractor);

      // reconstruct free blocks index from free block
      for (const auto& [key, size] : retval.m_freeBlocksIndex)
      {
         assert(size <= k_maxContiguousAllocation);
         retval.m_freeBlocks.insert({size, key});
      }
   }
   else
   {
      assert(spaceUsedInLastSegment == 0);
   }

   return retval;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::AllocatedSequence
Store<T, K, SegmentSizeBits>::getFirstAllocatedSequence() const noexcept
{
   Store<T, K, SegmentSizeBits>::AllocatedSequence allocatedSequence = {};

   if (!m_segment.empty())
   {
      allocatedSequence.key     = 0;
      allocatedSequence.size    = k_firstKeyValue;
      allocatedSequence.isUsed  = 0;
      allocatedSequence.isValid = 1;

      return getNextAllocatedSequence(allocatedSequence);
   }

   return allocatedSequence;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::AllocatedSequence Store<T, K, SegmentSizeBits>::getNextAllocatedSequence(
   Store<T, K, SegmentSizeBits>::AllocatedSequence allocatedSequence) const noexcept
{
   if (allocatedSequence.isValid)
   {
      do
      {
         allocatedSequence = getNextBlock(allocatedSequence);
      } while ((allocatedSequence.isValid == 1) && (allocatedSequence.isUsed == 0));
   }

   return allocatedSequence;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::AllocatedSequence
Store<T, K, SegmentSizeBits>::getNextBlock(Store<T, K, SegmentSizeBits>::AllocatedSequence allocatedSequence) const
   noexcept
{
   if (allocatedSequence.isValid == 0)
   {
      allocatedSequence.key  = 0;
      allocatedSequence.size = 0;
      return allocatedSequence;
   }

   const K               endOfThisAllocatedSequence{allocatedSequence.key + allocatedSequence.size};
   const block_size_type offsetInSegment{getOffsetInSegment(endOfThisAllocatedSequence)};

   if (offsetInSegment != 0U)
   {
      return getFirstBlockFollowing(endOfThisAllocatedSequence);
   }

   /*
    * This block runs until the end of the segment, so the next block must
    * be in the next segment.
    */
   const block_size_type segmentIndex{getSegmentIndex(endOfThisAllocatedSequence)};

   // no need to increment since getSegmentIndex wraps to the next segment
   return getFirstBlockInSegment(segmentIndex);
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::AllocatedSequence
Store<T, K, SegmentSizeBits>::getFirstBlockInSegment(block_size_type segmentIndex) const noexcept
{
   Store<T, K, SegmentSizeBits>::AllocatedSequence allocatedSequence = {};

   if (segmentIndex < m_segment.size())
   {
      const K key = makeKey(segmentIndex, k_firstKeyValue);

      return getFirstBlockFollowing(key);
   }

   return allocatedSequence;
}

template <typename T, typename K, unsigned SegmentSizeBits>
typename Store<T, K, SegmentSizeBits>::AllocatedSequence
Store<T, K, SegmentSizeBits>::getFirstBlockFollowing(K key) const noexcept
{
   Store<T, K, SegmentSizeBits>::AllocatedSequence allocatedSequence = {};

   const block_size_type segmentIndex{getSegmentIndex(key)};

   allocatedSequence.key = key;

   /*
    * There is something in this segment, free or allocated.
    */
   allocatedSequence.isValid = 1;

   const auto nextFreeBlockIter = m_freeBlocksIndex.upper_bound(key - 1);

   if (nextFreeBlockIter == m_freeBlocksIndex.end())
   {
      /*
       * There are no free blocks after this. So this is an allocated block...
       */
      allocatedSequence.isUsed = 1;

      /*
       * ... that extends to the end of the segment.
       */
      allocatedSequence.size = k_maxContiguousAllocation;
   }
   else
   {
      if (nextFreeBlockIter->first == allocatedSequence.key)
      {
         /*
          * Right here we start a free block, and ...
          */
         allocatedSequence.isUsed = 0;

         /*
          * ... its size is the size of the free block.
          */
         allocatedSequence.size = nextFreeBlockIter->second;
      }
      else
      {
         /*
          * There is a free block following this block, so this block is used.
          */
         allocatedSequence.isUsed = 1;

         const block_size_type nextFreeBlockSegmentIndex{getSegmentIndex(nextFreeBlockIter->first)};

         if (nextFreeBlockSegmentIndex == segmentIndex)
         {
            /*
             * The following free block is in the same segment, so the size
             * of this used block is just the distance to it.
             */
            allocatedSequence.size = nextFreeBlockIter->first - allocatedSequence.key;
         }
         else
         {
            /*
             * The following free block is in a different segment, so this
             * allocated block runs for the entirety of the segment.
             */
            allocatedSequence.size = k_maxContiguousAllocation;
         }
      }
   }

   return allocatedSequence;
}

template <typename T, typename K, unsigned SegmentSizeBits>
void Store<T, K, SegmentSizeBits>::validateInternalState() const
{
#ifdef FTAGS_STRICT_CHECKING
   assert(m_freeBlocks.size() == m_freeBlocksIndex.size());
#if 0
   std::vector<Block> freeBlocks(m_freeBlocks);

   std::sort(freeBlocks.begin(), freeBlocks.end(), [](const Block& left, const Block& right) -> bool {
      return left.key < right.key;
   });

   block_size_type currentSegment{k_maxSegmentCount + 1};
   block_size_type currentOffset{0};

   for (const auto& block : freeBlocks)
   {
      const block_size_type segmentIndex{getSegmentIndex(blockKey)};
      const block_size_type offsetInSegment{getOffsetInSegment(blockKey)};

      if (currentSegment != segmentIndex)
      {
         currentSegment = segmentIndex;
         currentOffset  = 0;
      }
      else
      {
         assert(currentOffset <= offsetInSegment);
         if (currentOffset > offsetInSegment)
         {
            throw std::logic_error("Internal free blocks corruption");
         }

         currentOffset = offsetInSegment + blockSize;
      }
   }
#endif
#endif
}

} // namespace ftags::util

#endif // STORE_H_INCLUDED
