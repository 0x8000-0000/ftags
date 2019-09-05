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

#include <statistics.h>

#include <fmt/format.h>

#include <random>

ftags::RecordSpanManager::Key ftags::RecordSpanManager::addSpan(const std::vector<Record>& records)
{
   RecordSpan::Hash hashValue = RecordSpan::computeHash(records);

   auto [beginRange, endRange] = m_cache.equal_range(hashValue);

   for (auto iter = beginRange; iter != endRange; ++iter)
   {
      const RecordSpan::Store::Key match = iter->second;

      const auto& [spanIter, spanRangeEnd] = m_recordSpanStore.get(match);

      if (spanIter->isEqualTo(records))
      {
         spanIter->addRef();
         return match;
      }
   }

   auto [newSpanKey, newSpanIterator] = m_recordSpanStore.allocate(1);
   memset(static_cast<void*>(newSpanIterator), 0, sizeof(RecordSpan));
   newSpanIterator->setRecordsFrom(records, m_recordStore, m_symbolIndexStore);
   newSpanIterator->addRef();

   m_cache.emplace(hashValue, newSpanKey);

   indexRecordSpan(*newSpanIterator, newSpanKey);

   return newSpanKey;
}

void ftags::RecordSpanManager::indexRecordSpan(const ftags::RecordSpan&      recordSpan,
                                               ftags::RecordSpan::Store::Key recordSpanKey)
{
   // gather all unique symbols in this record span
   std::set<ftags::util::StringTable::Key> symbolKeys;
   recordSpan.forEachRecord([&symbolKeys](const Record* record) { symbolKeys.insert(record->symbolNameKey); });

   /*
    * add a mapping from this symbol to this record span
    */
   std::for_each(
      symbolKeys.cbegin(), symbolKeys.cend(), [this, recordSpanKey](ftags::util::StringTable::Key symbolKey) {
         m_symbolIndex.emplace(symbolKey, recordSpanKey);
      });

   m_fileIndex.emplace(recordSpan.getFileKey(), recordSpanKey);
}

std::size_t ftags::RecordSpanManager::computeSerializedSize() const
{
   return sizeof(ftags::util::SerializedObjectHeader) + m_recordSpanStore.computeSerializedSize() +
          m_recordStore.computeSerializedSize();
}

void ftags::RecordSpanManager::serialize(ftags::util::BufferInsertor& insertor) const
{
   assertValid();

   ftags::util::SerializedObjectHeader header{"ftags::RecordSpanManager"};
   insertor << header;

   m_recordSpanStore.serialize(insertor);
   m_recordStore.serialize(insertor);
}

ftags::RecordSpanManager ftags::RecordSpanManager::deserialize(ftags::util::BufferExtractor& extractor)
{
   ftags::util::SerializedObjectHeader header = {};
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

std::set<ftags::util::StringTable::Key> ftags::RecordSpanManager::getSymbolKeys() const
{
   std::set<ftags::util::StringTable::Key> uniqueKeys;
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

std::vector<const ftags::Record*>
ftags::RecordSpanManager::findClosestRecord(ftags::util::StringTable::Key   fileNameKey,
                                            const ftags::util::StringTable& symbolTable,
                                            unsigned                        lineNumber,
                                            unsigned                        columnNumber) const
{
   std::vector<const ftags::Record*> recordsOnLine = filterRecordsFromFile(
      fileNameKey, [lineNumber](const Record* record) { return record->location.line == lineNumber; });

   std::sort(recordsOnLine.begin(), recordsOnLine.end(), [](const Record* left, const Record* right) -> bool {
      return left->location.column < right->location.column;
   });

   std::vector<const ftags::Record*> results;
   results.reserve(recordsOnLine.size());

   for (const auto record : recordsOnLine)
   {
      const std::size_t symbolLength = symbolTable.getStringView(record->symbolNameKey).size();
      if ((record->location.column <= columnNumber) && (columnNumber <= (record->location.column + symbolLength)))
      {
         results.push_back(record);
      }
   }

   return results;
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

std::vector<std::string> ftags::RecordSpanManager::getStatisticsRemarks() const noexcept
{
   ftags::stats::Sample<unsigned> usageCount;
   ftags::stats::Sample<unsigned> spanSizes;

   m_recordSpanStore.forEachAllocatedSequence([&usageCount, &spanSizes](RecordSpan::Store::Key /* key */,
                                                                        const RecordSpan*                  recordSpan,
                                                                        RecordSpan::Store::block_size_type size) {
      for (RecordSpan::Store::block_size_type ii = 0; ii < size; ii++)
      {
         auto usage = recordSpan[ii].getUsage();
         assert(usage >= 0);
         usageCount.addValue(static_cast<unsigned>(usage));
         spanSizes.addValue(recordSpan[ii].getSize());
      }
   });

   const ftags::stats::FiveNumbersSummary<unsigned> usageCountSummary = usageCount.computeFiveNumberSummary();
   const ftags::stats::FiveNumbersSummary<unsigned> spanSizesSummary  = spanSizes.computeFiveNumberSummary();

   std::vector<std::string> remarks;

   remarks.emplace_back(fmt::format("Record span count: {:n}", usageCount.getSampleCount()));
   remarks.emplace_back("");

   remarks.emplace_back("Record span sizes, (five number summary):");
   remarks.emplace_back(fmt::format("  minimum:        {:>10n}", spanSizesSummary.minimum));
   remarks.emplace_back(fmt::format("  lower quartile: {:>10n}", spanSizesSummary.lowerQuartile));
   remarks.emplace_back(fmt::format("  median:         {:>10n}", spanSizesSummary.median));
   remarks.emplace_back(fmt::format("  upper quartile: {:>10n}", spanSizesSummary.upperQuartile));
   remarks.emplace_back(fmt::format("  maximum:        {:>10n}", spanSizesSummary.maximum));
   remarks.emplace_back("");

   remarks.emplace_back("Record span usage, (five number summary):");
   remarks.emplace_back(fmt::format("  minimum:        {:>10n}", usageCountSummary.minimum));
   remarks.emplace_back(fmt::format("  lower quartile: {:>10n}", usageCountSummary.lowerQuartile));
   remarks.emplace_back(fmt::format("  median:         {:>10n}", usageCountSummary.median));
   remarks.emplace_back(fmt::format("  upper quartile: {:>10n}", usageCountSummary.upperQuartile));
   remarks.emplace_back(fmt::format("  maximum:        {:>10n}", usageCountSummary.maximum));
   remarks.emplace_back("");

   return remarks;
}

std::vector<std::string>
ftags::RecordSpanManager::analyzeRecordSpans(const ftags::util::StringTable& /* symbolTable */,
                                             const ftags::util::StringTable& fileNameTable) const noexcept
{
   std::vector<const RecordSpan*> spans;

   m_recordSpanStore.forEachAllocatedSequence([&spans](RecordSpan::Store::Key /* key */,
                                                       const RecordSpan*                  recordSpan,
                                                       RecordSpan::Store::block_size_type size) {
      for (RecordSpan::Store::block_size_type ii = 0; ii < size; ii++)
      {
         spans.push_back(&recordSpan[ii]);
      }
   });

   auto compareRecordSpansBySize = [](const RecordSpan* left, const RecordSpan* right) -> bool {
      return left->getSize() < right->getSize();
   };

   std::sort(spans.begin(), spans.end(), compareRecordSpansBySize);

   auto spansEnd = spans.end();

   auto iter = spans.begin();

   std::vector<std::string> remarks;
   remarks.emplace_back("Single records:");

   std::random_device randomDevice;
   std::mt19937       generator(randomDevice());

   const unsigned maximumRecordsToDump = 128;

   while (iter != spansEnd)
   {
      auto rangeEnd = std::upper_bound(iter, spansEnd, *iter, compareRecordSpansBySize);

      remarks.emplace_back(fmt::format("Records of size {}", (*iter)->getSize()));

      /*
       * Now all spans from iter to rangeEnd have the same size
       */
      std::shuffle(iter, rangeEnd, generator);

      while ((remarks.size() < maximumRecordsToDump) && (iter != rangeEnd))
      {
         (*iter)->forEachRecord([&remarks, &fileNameTable](const Record* record) {
            if (remarks.size() < maximumRecordsToDump)
            {
               remarks.emplace_back(fmt::format("{}:{}:{}",
                                                fileNameTable.getStringView(record->location.fileNameKey),
                                                record->location.line,
                                                record->location.column));
            }
         });

         ++iter;
      }

      if (remarks.size() >= maximumRecordsToDump)
      {
         break;
      }

      iter = rangeEnd;
   }

   return remarks;
}

std::vector<std::string> ftags::RecordSpanManager::analyzeRecords() const noexcept
{
   std::vector<std::string> remarks;
   return remarks;
}
