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
         ftags::StringTable::Key key;
      };

      RecordSymbolComparator(const Record* records) : m_records{records}
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
   using Store = ftags::Store<RecordSpan, uint32_t, 20>;
   using Hash  = std::uint64_t;

   RecordSpan(ftags::Record::Store::Key key, std::uint32_t size, ftags::Record* recordBase) :
      m_key{key},
      m_size{size},
      m_records{recordBase}
   {
   }

   RecordSpan(std::uint32_t size, ftags::Record::Store& store) : m_size{size}
   {
      auto alloc = store.allocate(size);
      m_key      = alloc.key;
      m_records  = &*alloc.iterator;
   }

   RecordSpan(RecordSpan&& other) :
      m_key{other.m_key},
      m_size{other.m_size},
      m_records{other.m_records},
      m_hash{other.m_hash},
      m_recordsInSymbolKeyOrder(other.m_recordsInSymbolKeyOrder)
   {
   }

   ftags::Record::Store::Key getKey() const
   {
      return m_key;
   }

   ftags::Record::Store::Key getSize() const
   {
      return m_size;
   }

   Hash getHash() const
   {
      return m_hash;
   }

   static Hash computeHash(const std::vector<Record>& records);

   void copyRecordsFrom(const std::vector<ftags::Record>& other);

   void copyRecordsFrom(const RecordSpan& other);

   void copyRecordsTo(std::vector<Record>& newCopy);

   static void
   filterRecords(std::vector<Record>&                                                    records,
                 const ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key>& symbolKeyMapping,
                 const ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key>& fileNameKeyMapping);

   template <typename F>
   void forEachRecord(F func) const
   {
      for (std::uint32_t ii = 0; ii < m_size; ii++)
      {
         func(&m_records[ii]);
      }
   }

   template <typename F>
   void forEachRecordWithSymbol(ftags::StringTable::Key symbolNameKey, F func) const
   {
      const RecordSymbolComparator::KeyWrapper keyWrapper{symbolNameKey};

      const auto keyRange = std::equal_range(m_recordsInSymbolKeyOrder.cbegin(),
                                             m_recordsInSymbolKeyOrder.cend(),
                                             keyWrapper,
                                             RecordSymbolComparator(m_records));

      for (auto iter = keyRange.first; iter != keyRange.second; ++iter)
      {
         func(&m_records[*iter]);
      }
   }

   /*
    * Debugging
    */
   void dumpRecords(std::ostream&             os,
                    const ftags::StringTable& symbolTable,
                    const ftags::StringTable& fileNameTable) const;

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static RecordSpan deserialize(ftags::BufferExtractor& extractor, ftags::Record::Store& recordStore);

private:
   // persistent data
   ftags::Record::Store::Key m_key  = 0;
   std::uint32_t             m_size = 0;

   // cached value
   Record* m_records = nullptr;

   int m_referenceCount = 0;

   // 128 bit hash of the record span
   Hash m_hash = 0;

   std::vector<uint32_t> m_recordsInSymbolKeyOrder;

   static constexpr uint64_t k_hashSeed = 0x0accedd62cf0b9bf;

   void updateIndices();
};

} // namespace ftags

#endif // FTAGS_DB_RECORD_SPAN_H_INCLUDED
