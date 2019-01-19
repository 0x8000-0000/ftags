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

#include <string_table.h>

#include <algorithm>
#include <iterator>
#include <stdexcept>

#include <cassert>
#include <cstring>

void ftags::StringTable::addBucket()
{
   if ((m_buckets.size() + 1) >= MaxBucketCount)
   {
      throw std::length_error("Exceeded data structure capacity");
   }

   m_buckets.emplace_back(std::vector<char>());
   m_buckets.back().reserve(MaxTableSize);
   m_buckets.back().push_back('\0');
}

const char* ftags::StringTable::getString(uint32_t stringKey) const noexcept
{
   const uint32_t positionMask    = MaxBucketCount - 1;
   const uint32_t bucketIndexMask = ~positionMask;

   const auto bucketIndex = (stringKey & bucketIndexMask) >> BucketSizeBits;
   const auto position    = stringKey & positionMask;

   const auto& bucket = m_buckets.at(bucketIndex);
   assert(position < bucket.size());

   auto iter = bucket.begin();

   std::advance(iter, position);

   return &*iter;
}

uint32_t ftags::StringTable::getKey(const char* inputString) const noexcept
{
   auto iter = m_registry.find(inputString);
   if (m_registry.end() == iter)
   {
      return 0;
   }
   else
   {
      return iter->second;
   }
}

uint32_t ftags::StringTable::addKey(const char* inputString)
{
   const uint32_t currentPosition = getKey(inputString);
   if (currentPosition != 0)
   {
      return currentPosition;
   }

   const auto inputLength = strlen(inputString);
   if (inputLength >= MaxTableSize)
   {
      throw std::length_error("Can't store strings that large");
   }

   auto& currentBucket = m_buckets.back();

   if ((currentBucket.size() + inputLength + 1) <= currentBucket.capacity())
   {
      const size_t position = currentBucket.size();
      assert(position < MaxTableSize);

      std::copy_n(inputString, inputLength + 1, std::back_inserter(currentBucket));

      const long bucketIndex = std::distance(m_buckets.begin(), m_buckets.end());
      assert(bucketIndex >= 0);
      assert(bucketIndex < (1u << (32 - BucketSizeBits)));

      const uint32_t stringKey =
         (static_cast<uint32_t>(bucketIndex) << (32 - BucketSizeBits)) | static_cast<uint32_t>(position);

      m_registry[inputString] = stringKey;

      return static_cast<uint32_t>(stringKey);
   }
   else
   {
      addBucket();

      return addKey(inputString);
   }
}

std::vector<uint8_t> ftags::StringTable::serialize() const
{
   std::vector<uint8_t> retval;
   return retval;
}

ftags::StringTable ftags::StringTable::deserialize(const uint8_t* /* buffer */, size_t /* size */)
{
   ftags::StringTable retval;
   return retval;
}
