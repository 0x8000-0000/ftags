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

#include <record.h>

namespace
{

class OrderRecordsBySymbolKey
{
public:
   explicit OrderRecordsBySymbolKey(const ftags::Record* records) : m_records{records}
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
   explicit OrderRecordsByFileKey(const std::vector<ftags::Record>& records) : m_records{records}
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

} // anonymous namespace

void ftags::Record::filterDuplicates(std::vector<const ftags::Record*>& records)
{
   const auto begin = records.begin();
   const auto end   = records.end();

   std::sort(begin, end, [](const ftags::Record* leftRecord, const ftags::Record* rightRecord) {
      return *leftRecord < *rightRecord;
   });

   auto last = std::unique(begin, end, [](const ftags::Record* leftRecord, const ftags::Record* rightRecord) {
      return *leftRecord == *rightRecord;
   });
   records.erase(last, end);
}

/*
 * Record serialization
 */
template <>
std::size_t
ftags::util::Serializer<std::vector<ftags::Record>>::computeSerializedSize(const std::vector<ftags::Record>& val)
{
   return sizeof(ftags::util::SerializedObjectHeader) + sizeof(uint64_t) + val.size() * sizeof(ftags::Record);
}

template <>
void ftags::util::Serializer<std::vector<ftags::Record>>::serialize(const std::vector<ftags::Record>& val,
                                                                    ftags::util::TypedInsertor&       insertor)
{
   ftags::util::SerializedObjectHeader header{"std::vector<ftags::Record>"};
   insertor << header;

   const uint64_t vecSize = val.size();
   insertor << vecSize;

   insertor << val;
}

template <>
std::vector<ftags::Record>
ftags::util::Serializer<std::vector<ftags::Record>>::deserialize(ftags::util::TypedExtractor& extractor)
{
   ftags::util::SerializedObjectHeader header;
   extractor >> header;

   uint64_t vecSize = 0;
   extractor >> vecSize;

   std::vector<ftags::Record> retval(/* __n = */ vecSize);
   extractor >> retval;

   return retval;
}

#if 0
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
         recordCount += val->getSize();
      }
   }
   return recordCount;
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
   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) + recordSpanSizes +
          m_store.computeSerializedSize();
}

void ftags::RecordSpanCache::serialize(ftags::TypedInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::RecordSpanCache"};
   insertor << header;

   m_store.serialize(insertor);

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

ftags::RecordSpanCache ftags::RecordSpanCache::deserialize(ftags::TypedExtractor&                   extractor,
                                                           std::vector<std::shared_ptr<RecordSpan>>& hardReferences)
{
   ftags::RecordSpanCache retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   retval.m_store = RecordStore::deserialize(extractor);

   uint64_t cacheSize = 0;
   extractor >> cacheSize;

   retval.m_cache.reserve(cacheSize);
   hardReferences.reserve(cacheSize);

   for (size_t ii = 0; ii < cacheSize; ii++)
   {
      RecordSpan recordSpan = RecordSpan::deserialize(extractor, retval.m_store);

      std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>(std::move(recordSpan));
      hardReferences.push_back(newSpan);

      retval.m_cache.emplace(newSpan->getHash(), newSpan);

      retval.indexRecordSpan(newSpan);
   }

   return retval;
}

std::shared_ptr<ftags::RecordSpan> ftags::RecordSpanCache::getSpan(const std::vector<Record>& records)
{
   const RecordSpan::Hash spanHash = RecordSpan::computeHash(records);

   cache_type::const_iterator iter = m_cache.find(spanHash);
   if (iter != m_cache.end())
   {
      return iter->second.lock();
   }
   else
   {
      std::shared_ptr<RecordSpan> newSpan = makeEmptySpan(records.size());
      newSpan->copyRecordsFrom(records);
      m_cache.emplace(newSpan->getHash(), newSpan);
      indexRecordSpan(newSpan);
      return newSpan;
   }
}
#endif
