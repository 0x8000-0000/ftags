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

#ifndef FTAGS_DB_RECORD_SPAN_MANAGER_H_INCLUDED
#define FTAGS_DB_RECORD_SPAN_MANAGER_H_INCLUDED

#include <record.h>
#include <record_span.h>

#include <string_table.h>

#include <map>

namespace ftags
{

class RecordSpanManager
{
   using Index = std::multimap<ftags::util::StringTable::Key, RecordSpan::Store::Key>;

   /** Maps from a symbol key to a bag of translation units containing the symbol.
    */
   Index m_symbolIndex;

   Index m_fileIndex;

public:
   RecordSpanManager() = default;

   RecordSpanManager(const ftags::RecordSpanManager& other) = delete;
   const RecordSpanManager& operator=(const ftags::RecordSpanManager& other) = delete;

   RecordSpanManager(RecordSpanManager&& other) noexcept :
      m_symbolIndex{std::move(other.m_symbolIndex)},
      m_recordSpanStore{std::move(other.m_recordSpanStore)},
      m_recordStore{std::move(other.m_recordStore)},
      m_cache{std::move(other.m_cache)},
      m_symbolIndexStore{std::move(other.m_symbolIndexStore)}
   {
   }

   RecordSpanManager& operator=(RecordSpanManager&& other) noexcept
   {
      m_symbolIndex      = std::move(other.m_symbolIndex);
      m_recordSpanStore  = std::move(other.m_recordSpanStore);
      m_recordStore      = std::move(other.m_recordStore);
      m_cache            = std::move(other.m_cache);
      m_symbolIndexStore = std::move(other.m_symbolIndexStore);

      return *this;
   }

   ~RecordSpanManager() = default;

   using Key = ftags::RecordSpan::Store::Key;

   Key addSpan(const std::vector<Record>& records);

   const ftags::RecordSpan& getSpan(Key key) const
   {
      if (key == 0U)
      {
         assert(key != 0);
         throw(std::runtime_error("Invalid span key"));
      }

      const auto spanIterPair = m_recordSpanStore.get(key);
      return *spanIterPair.first;
   }

   ftags::RecordSpan& getSpan(Key key)
   {
      if (key == 0U)
      {
         assert(key != 0);
         throw(std::runtime_error("Invalid span key"));
      }

      auto spanIterPair = m_recordSpanStore.get(key);
      return *spanIterPair.first;
   }

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::util::TypedInsertor& insertor) const;

   static ftags::RecordSpanManager deserialize(ftags::util::TypedExtractor& extractor);

   /*
    * Query interface
    */
   template <typename F>
   std::vector<const Record*> filterRecords(F                               selectRecord,
                                            const ftags::util::StringTable& symbolNames,
                                            const ftags::util::StringTable& fileNames) const
   {
      std::vector<const ftags::Record*> results;

      m_recordStore.forEach(
         [&results, &selectRecord, &symbolNames, &fileNames](Record::Store::Key /* key */, const Record* record) {
            if (selectRecord(record, symbolNames, fileNames))
            {
               results.push_back(record);
            }
         });

      return results;
   }

   template <typename F>
   std::vector<const Record*> filterRecordsWithSymbol(ftags::util::StringTable::Key symbolKey, F selectRecord) const
   {
      std::vector<const ftags::Record*> results;

      if (symbolKey)
      {
         const auto range = m_symbolIndex.equal_range(symbolKey);
         for (auto iter = range.first; iter != range.second; ++iter)
         {
            const RecordSpan& recordSpan = getSpan(iter->second);

            recordSpan.forEachRecordWithSymbol(
               symbolKey,
               [&results, selectRecord](const Record* record) {
                  if (selectRecord(record))
                  {
                     results.push_back(record);
                  }
               },
               m_symbolIndexStore);
         }
      }

      return results;
   }

   template <typename F>
   void forEachRecordWithSymbol(ftags::util::StringTable::Key symbolKey, F func) const
   {
      if (symbolKey)
      {
         const auto range = m_symbolIndex.equal_range(symbolKey);
         for (auto iter = range.first; iter != range.second; ++iter)
         {
            const RecordSpan& recordSpan = getSpan(iter->second);

            recordSpan.forEachRecordWithSymbol(symbolKey, func, m_symbolIndexStore);
         }
      }
   }

   template <typename F>
   void forEachRecord(F func) const
   {
      m_recordStore.forEach([func](Record::Store::Key /* key */, const Record* record) {
         if (record->symbolNameKey != 0)
         {
            func(record);
         }
      });
   }

   template <typename F>
   std::vector<const Record*> filterRecordsFromFile(ftags::util::StringTable::Key fileNameKey, F selectRecord) const
   {
      std::vector<const ftags::Record*> results;
      if (fileNameKey)
      {
         const auto range = m_fileIndex.equal_range(fileNameKey);
         for (auto iter = range.first; iter != range.second; ++iter)
         {
            const RecordSpan& recordSpan = getSpan(iter->second);

            recordSpan.forEachRecord([&results, selectRecord](const Record* record) {
               if (selectRecord(record))
               {
                  results.push_back(record);
               }
            });
         }
      }
      return results;
   }

   std::vector<const Record*> findClosestRecord(ftags::util::StringTable::Key   fileNameKey,
                                                const ftags::util::StringTable& symbolTable,
                                                unsigned                        lineNumber,
                                                unsigned                        columnNumber) const;

   std::size_t getRecordCount() const
   {
      return m_recordStore.countUsedBlocks();
   }

   std::size_t getSymbolCount() const;

   void assertValid() const
#if defined(NDEBUG) || (!defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
   {
   }
#else
      ;
#endif

   std::vector<std::string> getStatisticsRemarks() const noexcept;

   std::vector<std::string> analyzeRecordSpans(const ftags::util::StringTable& symbolTable,
                                               const ftags::util::StringTable& fileNameTable) const noexcept;

   std::vector<std::string> analyzeRecords() const noexcept;

private:
   // persistent
   ftags::RecordSpan::Store m_recordSpanStore;
   ftags::Record::Store     m_recordStore;

   /*
    * transient
    */

   using Cache = std::multimap<RecordSpan::Hash, RecordSpan::Store::Key>;

   /* stores a cache from the hash value of the records in a span
    * to the span key itself so we can cheaply find span duplicates
    */
   Cache m_cache;

   /* for each record span, stores the indices of the records within, in
    * sorted by the symbol key order, to speed-up lookups by symbol
    */
   RecordSpan::SymbolIndexStore m_symbolIndexStore;

   void indexRecordSpan(const RecordSpan& recordSpan, RecordSpan::Store::Key key);

   std::set<ftags::util::StringTable::Key> getSymbolKeys() const;
};

} // namespace ftags

#endif // FTAGS_DB_RECORD_SPAN_MANAGER_H_INCLUDED
