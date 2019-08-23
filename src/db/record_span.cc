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

#include <record_span.h>

#include <SpookyV2.h>

#include <numeric>

namespace
{

class OrderRecordsBySymbolKey
{
public:
   OrderRecordsBySymbolKey(const ftags::Record* records) : m_records{records}
   {
   }

   bool operator()(const unsigned& left, const unsigned& right) const
   {
      const ftags::Record& leftRecord  = m_records[left];
      const ftags::Record& rightRecord = m_records[right];

      if (leftRecord.symbolNameKey < rightRecord.symbolNameKey)
      {
         return true;
      }

      if (leftRecord.symbolNameKey == rightRecord.symbolNameKey)
      {
         if (leftRecord.attributes.type < rightRecord.attributes.type)
         {
            return true;
         }
      }

      return false;
   }

private:
   const ftags::Record* m_records;
};

} // anonymous namespace

void ftags::RecordSpan::updateIndices()
{
   m_recordsInSymbolKeyOrder.resize(m_size);
   std::iota(m_recordsInSymbolKeyOrder.begin(), m_recordsInSymbolKeyOrder.end(), 0);
   std::sort(m_recordsInSymbolKeyOrder.begin(), m_recordsInSymbolKeyOrder.end(), OrderRecordsBySymbolKey(m_records));
}

void ftags::RecordSpan::copyRecordsFrom(const RecordSpan& other)
{
   assert(m_size == other.m_size);
   memcpy(m_records, other.m_records, m_size * sizeof(Record));

   updateIndices();

#ifndef NDEBUG
   m_hash = SpookyHash::Hash64(m_records, m_size * sizeof(Record), k_hashSeed);
   assert(m_hash == other.m_hash);
#else
   m_hash = other.m_hash;
#endif
}

void ftags::RecordSpan::copyRecordsFrom(const std::vector<Record>& other)
{
   assert(m_size == other.size());
   memcpy(m_records, other.data(), m_size * sizeof(Record));

   updateIndices();

   m_hash = SpookyHash::Hash64(m_records, m_size * sizeof(Record), k_hashSeed);
}

void ftags::RecordSpan::copyRecordsTo(std::vector<Record>& newCopy)
{
   newCopy.resize(m_size);
   memcpy(newCopy.data(), m_records, m_size * sizeof(Record));
}

void ftags::RecordSpan::filterRecords(
   std::vector<Record>&                                                    records,
   const ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key>& symbolKeyMapping,
   const ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key>& fileNameKeyMapping)
{
   for (Record& record : records)
   {
      {
         auto fileNameIter = fileNameKeyMapping.lookup(record.location.fileNameKey);
         assert(fileNameIter != fileNameKeyMapping.none());
         record.setLocationFileKey(fileNameIter->second);
      }

      {
         auto fileNameIter = fileNameKeyMapping.lookup(record.definition.fileNameKey);
         assert(fileNameIter != fileNameKeyMapping.none());
         record.setDefinitionFileKey(fileNameIter->second);
      }

      {
         auto symbolNameIter = symbolKeyMapping.lookup(record.symbolNameKey);
         assert(symbolNameIter != symbolKeyMapping.none());
         record.symbolNameKey = symbolNameIter->second;
      }
   }
}

ftags::RecordSpan::Hash ftags::RecordSpan::computeHash(const std::vector<Record>& records)
{
   return SpookyHash::Hash64(records.data(), records.size() * sizeof(Record), k_hashSeed);
}

std::size_t ftags::RecordSpan::computeSerializedSize() const
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(std::size_t) + sizeof(uint32_t) + sizeof(std::size_t);
}

void ftags::RecordSpan::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::RecordSpan"};
   insertor << header;

   insertor << m_size;
   insertor << m_key;
   insertor << m_hash;

#ifndef NDEBUG
   const std::size_t hash = SpookyHash::Hash64(m_records, m_size * sizeof(Record), k_hashSeed);
#endif
   assert(m_hash == hash);
}

ftags::RecordSpan ftags::RecordSpan::deserialize(ftags::BufferExtractor& extractor, ftags::Record::Store& recordStore)
{
   ftags::SerializedObjectHeader header;
   extractor >> header;

   uint32_t         size = 0;
   uint32_t         key  = 0;
   RecordSpan::Hash hash = 0;

   extractor >> size;
   extractor >> key;
   extractor >> hash;

   auto iter = recordStore.get(key);

   ftags::RecordSpan retval(/* key = */ key, /* size = */ size, /* records = */ &*iter.first);

   retval.m_hash = SpookyHash::Hash64(retval.m_records, retval.m_size * sizeof(Record), k_hashSeed);

   assert(hash == retval.m_hash);

   retval.updateIndices();

   return retval;
}
