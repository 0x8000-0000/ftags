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

#include <project.h>

#include <SpookyV2.h>

namespace
{

class OrderRecordsBySymbolKey
{
public:
   OrderRecordsBySymbolKey(const std::vector<ftags::Record>& records) : m_records{records}
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
   const std::vector<ftags::Record>& m_records;
};

bool compareRecordsByLocation(const ftags::Record& leftRecord, const ftags::Record& rightRecord)
{
   if (leftRecord.location.fileNameKey < rightRecord.location.fileNameKey)
   {
      return true;
   }

   if (leftRecord.location.fileNameKey == rightRecord.location.fileNameKey)
   {
      if (leftRecord.location.line < rightRecord.location.line)
      {
         return true;
      }

      if (leftRecord.location.line == rightRecord.location.line)
      {
         if (leftRecord.location.column < rightRecord.location.column)
         {
            return true;
         }
      }
   }

   return false;
}

class OrderRecordsByFileKey
{
public:
   OrderRecordsByFileKey(const std::vector<ftags::Record>& records) : m_records{records}
   {
   }

   bool operator()(const unsigned& left, const unsigned& right) const
   {
      const ftags::Record& leftRecord  = m_records[left];
      const ftags::Record& rightRecord = m_records[right];

      return compareRecordsByLocation(leftRecord, rightRecord);
   }

private:
   const std::vector<ftags::Record>& m_records;
};

#if 0
void filterDuplicates(std::vector<const ftags::Record*> records)
{
   const auto begin = records.begin();
   const auto end   = records.end();

   std::sort(begin, end, [](const ftags::Record* leftRecord, const ftags::Record* rightRecord) {
      return compareRecordsByLocation(*leftRecord, *rightRecord);
   });

   auto last = std::unique(begin, end);
   records.erase(last, end);
}
#endif

} // anonymous namespace

void ftags::RecordSpan::updateIndices()
{
   m_recordsInSymbolKeyOrder.resize(m_records.size());
   std::iota(m_recordsInSymbolKeyOrder.begin(), m_recordsInSymbolKeyOrder.end(), 0);
   std::sort(m_recordsInSymbolKeyOrder.begin(), m_recordsInSymbolKeyOrder.end(), OrderRecordsBySymbolKey(m_records));
}

void ftags::RecordSpan::addRecords(const RecordSpan& other)
{
   m_records = other.m_records;

   updateIndices();

   m_hash = SpookyHash::Hash64(m_records.data(), m_records.size() * sizeof(Record), k_hashSeed);
}

void ftags::RecordSpan::addRecords(const RecordSpan&               other,
                                   const ftags::FlatMap<Key, Key>& symbolKeyMapping,
                                   const ftags::FlatMap<Key, Key>& fileNameKeyMapping)
{
   m_records = other.m_records;

   for (auto& record : m_records)
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

   updateIndices();

   m_hash = SpookyHash::Hash64(m_records.data(), m_records.size() * sizeof(Record), k_hashSeed);
}

std::shared_ptr<ftags::RecordSpan> ftags::RecordSpanCache::add(std::shared_ptr<ftags::RecordSpan> original)
{
   m_spansObserved++;

   auto range = m_cache.equal_range(original->getHash());

   for (auto iter = range.first; iter != range.second; ++iter)
   {
      auto elem = iter->second;

      std::shared_ptr<RecordSpan> val = elem.lock();

      if (!val)
      {
         continue;
      }
      else
      {
         if (val->operator==(*original))
         {
            return val;
         }
      }
   }

   m_cache.emplace(original->getHash(), original);

   indexRecordSpan(original);

   return original;
}

void ftags::RecordSpanCache::indexRecordSpan(std::shared_ptr<ftags::RecordSpan> original)
{
   /*
    * gather all unique symbols in this record span
    */
   std::set<ftags::StringTable::Key> symbolKeys;
   original->forEachRecord([&symbolKeys](const Record* record) { symbolKeys.insert(record->symbolNameKey); });

   /*
    * add a mapping from this symbol to this record span
    */
   std::for_each(symbolKeys.cbegin(), symbolKeys.cend(), [this, original](ftags::StringTable::Key symbolKey) {
      m_symbolIndex.emplace(symbolKey, original);
   });
}

std::size_t ftags::RecordSpanCache::getRecordCount() const
{
   std::size_t recordCount = 0;
   for (auto iter = m_cache.begin(); iter != m_cache.end(); ++iter)
   {
      std::shared_ptr<RecordSpan> val = iter->second.lock();
      if (val)
      {
         recordCount += val->getRecordCount();
      }
   }
   return recordCount;
}

/*
 * Record serialization
 */
template <>
std::size_t
ftags::Serializer<std::vector<ftags::Record>>::computeSerializedSize(const std::vector<ftags::Record>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) + val.size() * sizeof(ftags::Record);
}

template <>
void ftags::Serializer<std::vector<ftags::Record>>::serialize(const std::vector<ftags::Record>& val,
                                                              ftags::BufferInsertor&            insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<ftags::Record>"};
   insertor << header;

   const uint64_t vecSize = val.size();
   insertor << vecSize;

   insertor << val;
}

template <>
std::vector<ftags::Record>
ftags::Serializer<std::vector<ftags::Record>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header;
   extractor >> header;

   uint64_t vecSize = 0;
   extractor >> vecSize;

   std::vector<ftags::Record> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}

std::size_t ftags::RecordSpan::computeSerializedSize() const
{
   return sizeof(ftags::SerializedObjectHeader) +
          ftags::Serializer<std::vector<Record>>::computeSerializedSize(m_records);
}

void ftags::RecordSpan::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::RecordSpan"};
   insertor << header;

   ftags::Serializer<std::vector<Record>>::serialize(m_records, insertor);
}

ftags::RecordSpan ftags::RecordSpan::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::RecordSpan retval{0};

   ftags::SerializedObjectHeader header;
   extractor >> header;

   retval.m_records = ftags::Serializer<std::vector<Record>>::deserialize(extractor);

   retval.m_hash = SpookyHash::Hash64(retval.m_records.data(), retval.m_records.size() * sizeof(Record), k_hashSeed);

   retval.updateIndices();

   return retval;
}

std::size_t ftags::RecordSpanCache::computeSerializedSize() const
{
   std::size_t recordSpanSizes = 0;
   for (auto iter = m_cache.begin(); iter != m_cache.end(); ++iter)
   {
      std::shared_ptr<RecordSpan> val = iter->second.lock();
      if (val)
      {
         recordSpanSizes += val->computeSerializedSize();
      }
   }
   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) + recordSpanSizes;
}

void ftags::RecordSpanCache::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::RecordSpanCache"};
   insertor << header;

   const uint64_t vecSize = m_cache.size();
   insertor << vecSize;

   std::for_each(m_cache.cbegin(), m_cache.cend(), [&insertor](const cache_type::value_type& iter) {
      std::shared_ptr<RecordSpan> val = iter.second.lock();
      if (val)
      {
         val->serialize(insertor);
      }
   });
}

ftags::RecordSpanCache ftags::RecordSpanCache::deserialize(ftags::BufferExtractor&                   extractor,
                                                           std::vector<std::shared_ptr<RecordSpan>>& hardReferences)
{
   ftags::RecordSpanCache retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   uint64_t cacheSize = 0;
   extractor >> cacheSize;

   retval.m_cache.reserve(cacheSize);
   hardReferences.reserve(cacheSize);

   for (size_t ii = 0; ii < cacheSize; ii++)
   {
      std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>(0);
      *newSpan                            = RecordSpan::deserialize(extractor);
      hardReferences.push_back(newSpan);

      retval.m_cache.emplace(newSpan->getHash(), newSpan);

      retval.indexRecordSpan(newSpan);
   }

   return retval;
}
