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

#ifndef FTAGS_DB_RECORD_SPAN_H_INCLUDED
#define FTAGS_DB_RECORD_SPAN_H_INCLUDED

#include <record.h>

#include <store.h>

#include <filesystem>
#include <string>
#include <vector>

namespace ftags
{

/** Contains all the symbols in a C++ translation unit that are adjacent in
 * a physical file.
 *
 * For example, an include file that does not include any other files would
 * define a record span. If a file includes another file, that creates at
 * least three record spans, one before the include, one for the included file
 * and one after the include.
 */
class RecordSpan
{
private:
   struct RecordSymbolComparator
   {
      struct KeyWrapper
      {
         ftags::util::StringTable::Key key;
      };

      explicit RecordSymbolComparator(const Record* records) : m_records{records}
      {
      }

      bool operator()(std::vector<Record>::size_type recordPos, KeyWrapper symbolNameKey)
      {
         return m_records[recordPos].symbolNameKey < symbolNameKey.key;
      }

      bool operator()(KeyWrapper symbolNameKey, std::vector<Record>::size_type recordPos)
      {
         return symbolNameKey.key < m_records[recordPos].symbolNameKey;
      }

      const Record* m_records;
   };

public:
   using Store            = ftags::util::Store<RecordSpan, uint32_t, 22>; // NOLINT
   using Hash             = std::uint64_t;
   using SymbolIndexStore = ftags::util::Store<uint32_t, uint32_t, 22>; // NOLINT

   RecordSpan() = default;

   RecordSpan(ftags::Record::Store::Key key, std::uint32_t size, ftags::Record* recordBase) :
      m_key{key},
      m_size{size},
      m_records{recordBase}
   {
   }

   RecordSpan(std::size_t size, ftags::Record::Store& store) : m_size{static_cast<uint32_t>(size)}
   {
      auto alloc = store.allocate(m_size);
      m_key      = alloc.key;
      m_records  = alloc.iterator;
   }

   int getUsage() const
   {
      return m_referenceCount;
   }

   void addRef()
   {
      m_referenceCount++;
   }

   int release()
   {
      assert(m_referenceCount);
      m_referenceCount--;
      return m_referenceCount;
   }

   ftags::Record::Store::Key getKey() const
   {
      return m_key;
   }

   ftags::util::StringTable::Key getFileKey() const;

   ftags::Record::Store::Key getSize() const
   {
      return m_size;
   }

   Hash getHash() const
   {
      return m_hash;
   }

   static Hash computeHash(const std::vector<Record>& records);

   void copyRecordsFrom(const std::vector<Record>& other, SymbolIndexStore& symbolIndexStore);

   void setRecordsFrom(const std::vector<Record>& other, Record::Store& store, SymbolIndexStore& symbolIndexStore);

   void moveRecordsFrom(RecordSpan& other);

   void copyRecordsFrom(const RecordSpan& other, SymbolIndexStore& symbolIndexStore);

   void copyRecordsTo(std::vector<Record>& newCopy) const;

   bool isEqualTo(const std::vector<ftags::Record>& records) const;

   static void filterRecords(
      std::vector<Record>&                                                                      records,
      const ftags::util::FlatMap<ftags::util::StringTable::Key, ftags::util::StringTable::Key>& symbolKeyMapping,
      const ftags::util::FlatMap<ftags::util::StringTable::Key, ftags::util::StringTable::Key>& fileNameKeyMapping);

   template <typename F>
   void forEachRecord(F func) const
   {
      for (std::uint32_t ii = 0; ii < m_size; ii++)
      {
         func(&m_records[ii]);
      }
   }

   template <typename F>
   void forEachRecordWithSymbol(ftags::util::StringTable::Key symbolNameKey,
                                F                             func,
                                const SymbolIndexStore&       symbolIndexStore) const
   {
      assert(m_symbolIndexKey != 0);
      if (m_symbolIndexKey != 0)
      {
         const auto symbolIndexIter              = symbolIndexStore.get(m_symbolIndexKey);
         const auto recordsInSymbolKeyOrderBegin = symbolIndexIter.first;
         const auto recordsInSymbolKeyOrderEnd   = recordsInSymbolKeyOrderBegin + m_size;

         const RecordSymbolComparator::KeyWrapper keyWrapper{symbolNameKey};

         const auto keyRange = std::equal_range(
            recordsInSymbolKeyOrderBegin, recordsInSymbolKeyOrderEnd, keyWrapper, RecordSymbolComparator(m_records));

         for (auto iter = keyRange.first; iter != keyRange.second; ++iter)
         {
            func(&m_records[*iter]);
         }
      }
   }

   /*
    * Debugging
    */
   void dumpRecords(std::ostream&                   os,
                    const ftags::util::StringTable& symbolTable,
                    const ftags::util::StringTable& fileNameTable,
                    const std::filesystem::path&    trimPath) const;

#if 0
   // handled in bulk by RecordSpanManager

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static RecordSpan deserialize(ftags::BufferExtractor& extractor,
                                 ftags::Record::Store&   recordStore,
                                 SymbolIndexStore&       symbolIndexStore);
#endif

   void restoreRecordPointer(Record::Store& recordStore);

   void updateIndices(SymbolIndexStore& symbolIndexStore);

   void assertValid() const
#if defined(NDEBUG) || (!defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
   {
   }
#else
      ;
#endif

private:
   // persistent data
   ftags::Record::Store::Key m_key  = 0;
   std::uint32_t             m_size = 0;

   // cached value
   Record* m_records = nullptr;

   int m_referenceCount = 0;

   // 64-bit hash of the record span
   Hash m_hash = 0;

   SymbolIndexStore::Key m_symbolIndexKey = 0;

   static constexpr uint64_t k_hashSeed = 0x0accedd62cf0b9bf;
};

} // namespace ftags

#endif // FTAGS_DB_RECORD_SPAN_H_INCLUDED
