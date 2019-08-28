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

#include <record_span_manager.h>

ftags::RecordSpanManager::Key ftags::RecordSpanManager::addSpan(const std::vector<Record>& records)
{
   RecordSpan::Hash hashValue = RecordSpan::computeHash(records);

   auto iterPair = m_cache.equal_range(hashValue);

   for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
   {
      const RecordSpan::Store::Key match = iter->second;

      auto spanIterPair = m_recordSpanStore.get(match);

      RecordSpan& span = *spanIterPair.first;

      if (span.isEqualTo(records))
      {
         span.addRef();
         return match;
      }
   }

   auto alloc = m_recordSpanStore.allocate(1);
   memset(static_cast<void*>(alloc.iterator), 0, sizeof(RecordSpan));
   RecordSpan& newSpan = *alloc.iterator;
   newSpan.setRecordsFrom(records, m_recordStore, m_symbolIndexStore);
   newSpan.addRef();

   m_cache.emplace(hashValue, alloc.key);

   indexRecordSpan(newSpan, alloc.key);

   return alloc.key;
}

void ftags::RecordSpanManager::indexRecordSpan(const ftags::RecordSpan& recordSpan, ftags::RecordSpan::Store::Key key)
{
   // gather all unique symbols in this record span
   std::set<ftags::StringTable::Key> symbolKeys;
   recordSpan.forEachRecord([&symbolKeys](const Record* record) { symbolKeys.insert(record->symbolNameKey); });

   /*
    * add a mapping from this symbol to this record span
    */
   std::for_each(symbolKeys.cbegin(), symbolKeys.cend(), [this, key](ftags::StringTable::Key symbolKey) {
      m_symbolIndex.emplace(symbolKey, key);
   });
}

std::size_t ftags::RecordSpanManager::computeSerializedSize() const
{
   return sizeof(ftags::SerializedObjectHeader) + m_recordSpanStore.computeSerializedSize() +
          m_recordStore.computeSerializedSize();
}

void ftags::RecordSpanManager::serialize(ftags::BufferInsertor& insertor) const
{
   assertValid();

   ftags::SerializedObjectHeader header{"ftags::RecordSpanManager"};
   insertor << header;

   m_recordSpanStore.serialize(insertor);
   m_recordStore.serialize(insertor);
}

ftags::RecordSpanManager ftags::RecordSpanManager::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   ftags::RecordSpanManager retval;
   retval.m_recordSpanStore = RecordSpan::Store::deserialize(extractor);
   retval.m_recordStore     = Record::Store::deserialize(extractor);

   retval.m_recordSpanStore.forEachAllocatedSequence(
      [&retval](RecordSpan::Store::Key key, RecordSpan* recordSpan, RecordSpan::Store::block_size_type size) {
         for (RecordSpan::Store::block_size_type ii = 0; ii < size; ii++)
         {
            RecordSpan::Hash hashValue = recordSpan[ii].getHash();
            retval.m_cache.emplace(hashValue, key + ii);

            recordSpan[ii].restoreRecordPointer(retval.m_recordStore);

            recordSpan[ii].updateIndices(retval.m_symbolIndexStore);

            retval.indexRecordSpan(recordSpan[ii], key + ii);
         }
      });

   retval.assertValid();

   return retval;
}

std::set<ftags::StringTable::Key> ftags::RecordSpanManager::getSymbolKeys() const
{
   std::set<ftags::StringTable::Key> uniqueKeys;
   for (const auto& iter : m_symbolIndex)
   {
      uniqueKeys.insert(iter.first);
   }
   return uniqueKeys;
}

std::size_t ftags::RecordSpanManager::getSymbolCount() const
{
   return getSymbolKeys().size();
}

#if (!defined(NDEBUG)) && (defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
void ftags::RecordSpanManager::assertValid() const
{
   for (const auto [hash, recordSpanKey] : m_cache)
   {
      const auto [spanIter, rangeEnd] = m_recordSpanStore.get(recordSpanKey);
      const RecordSpan& recordSpan    = *spanIter;

      recordSpan.assertValid();
   }

   std::set<ftags::StringTable::Key> uniqueKeysFromRecords;

   m_recordSpanStore.forEachAllocatedSequence([&uniqueKeysFromRecords](RecordSpan::Store::Key             key,
                                                                       const RecordSpan*                  recordSpan,
                                                                       RecordSpan::Store::block_size_type size) {
      for (RecordSpan::Store::block_size_type ii = 0; ii < size; ii++)
      {
         recordSpan[ii].forEachRecord(
            [&uniqueKeysFromRecords](const Record* record) { uniqueKeysFromRecords.insert(record->symbolNameKey); });
      }
   });

   assert(uniqueKeysFromRecords == getSymbolKeys());
}
#endif
