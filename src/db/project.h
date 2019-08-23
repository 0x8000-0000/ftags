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

#ifndef DB_PROJECT_H_INCLUDED
#define DB_PROJECT_H_INCLUDED

#include <record.h>
#include <record_span.h>

#include <serialization.h>
#include <string_table.h>

#include <algorithm>
#include <iosfwd>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cstdint>

namespace ftags
{


enum class Relation : uint8_t
{
   Association, // generic association

   // specific associations
   Declares,
   Defines,
   Extends,   // as in 'derived class' extends 'base class'
   Overrides, // as in virtual methods
   Overloads, // as in 'A' and 'B' are overloads

};


struct Cursor
{
   struct Location
   {
      const char*  fileName;
      unsigned int line;
      unsigned int column;
   };

   char*       symbolNamespace;
   const char* symbolName;
   const char* unifiedSymbol;

   Attributes attributes;

   Location location;
   Location definition;
};

class RecordSpanCache
{
private:
   using cache_type = std::unordered_map<RecordSpan::Hash, std::weak_ptr<RecordSpan>>;

   using value_type = cache_type::value_type;

   using iterator_type = cache_type::iterator;

   using index_type = std::multimap<StringTable::Key, std::weak_ptr<RecordSpan>>;

   using RecordStore = ftags::Store<Record, uint32_t, 24>;

   RecordStore m_store;

   cache_type m_cache;

   std::size_t m_spansObserved = 0;

   /** Maps from a symbol key to a bag of translation units containing the symbol.
    */
   index_type m_symbolIndex;

   void indexRecordSpan(std::shared_ptr<ftags::RecordSpan> original);

   std::shared_ptr<RecordSpan> makeEmptySpan(std::size_t size)
   {
      return std::make_shared<RecordSpan>(size, m_store);
   }

public:
   std::pair<index_type::const_iterator, index_type::const_iterator> getSpansForSymbol(StringTable::Key key) const
   {
      return m_symbolIndex.equal_range(key);
   }

   std::shared_ptr<RecordSpan> getSpan(std::size_t size, uint32_t key)
   {
      auto alloc = m_store.get(key);
      return std::make_shared<RecordSpan>(key, size, &*alloc.first);
   }

   std::shared_ptr<RecordSpan> getSpan(const std::vector<Record>& records);

   std::shared_ptr<RecordSpan> add(std::shared_ptr<RecordSpan> newSpan);

   std::shared_ptr<RecordSpan> get(RecordSpan::Hash spanHash) const
   {
      cache_type::const_iterator iter = m_cache.find(spanHash);

      if (iter != m_cache.end())
      {
         return iter->second.lock();
      }
      else
      {
         return std::shared_ptr<RecordSpan>(nullptr);
      }
   }

   std::size_t getActiveSpanCount() const
   {
      return m_cache.size();
   }

   std::size_t getTotalSpanCount() const
   {
      return m_spansObserved;
   }

   std::size_t getRecordCount() const;

   template <typename F>
   void forEachRecord(F func) const
   {
      std::for_each(m_cache.cbegin(), m_cache.cend(), [func](const cache_type::value_type& elem) {
         std::shared_ptr<RecordSpan> recordSpan = elem.second.lock();

         if (recordSpan)
         {
            recordSpan->forEachRecord(func);
         }
      });
   }

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static RecordSpanCache deserialize(ftags::BufferExtractor&                   extractor,
                                      std::vector<std::shared_ptr<RecordSpan>>& hardReferences);
};

using KeyMap = ftags::FlatMap<ftags::StringTable::Key, ftags::StringTable::Key>;

class CursorSet
{
public:
   CursorSet(std::vector<const Record*> records, const StringTable& symbolTable, const StringTable& fileNameTable);

   Cursor inflateRecord(const Record& record) const;

   std::vector<Record>::const_iterator begin() const
   {
      return m_records.cbegin();
   }

   std::vector<Record>::const_iterator end() const
   {
      return m_records.cend();
   }

   std::vector<Record>::size_type size() const
   {
      return m_records.size();
   }

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static CursorSet deserialize(ftags::BufferExtractor& extractor);

   std::size_t computeHash() const;

private:
   CursorSet() = default;

   // persistent data
   std::vector<Record> m_records;
   StringTable         m_symbolTable;
   StringTable         m_fileNameTable;

   static constexpr uint64_t k_hashSeed[] = {0x6905e06277e77c15, 0x27e6864cb5ff7d26};
};

class ProjectDb
{
public:
   /*
    * Construction and maintenance
    */
   ProjectDb(std::string_view name, std::string_view rootDirectory) : m_name{name}, m_root{rootDirectory}
   {
   }

   const std::string& getName() const
   {
      return m_name;
   }

   const std::string& getRoot() const
   {
      return m_root;
   }

   bool operator==(const ProjectDb& other) const;

   void removeTranslationUnit(const std::string& fileName);

   Cursor inflateRecord(const Record* record) const;

   CursorSet inflateRecords(const std::vector<const Record*>& records) const;

   /*
    * General queries
    */

   std::vector<const Record*> getFunctions() const;

   std::vector<const Record*> getClasses() const;

   std::vector<const Record*> getGlobalVariables() const;

   bool isFileIndexed(const std::string& fileName) const;

   /*
    * Specific queries
    */

   std::vector<const Record*> findDeclaration(const std::string& symbolName) const;

   std::vector<const Record*> findDeclaration(const std::string& symbolName, SymbolType type) const;

   std::vector<const Record*> findDefinition(const std::string& symbolName) const;

   std::vector<const Record*> findReference(const std::string& symbolName) const;

   std::vector<const Record*> findSymbol(const std::string& symbolName) const
   {
      return filterRecordsWithSymbol(symbolName, [](const Record* /* record */) { return true; });
   }

   std::vector<const Record*> findSymbol(const std::string& symbolName, ftags::SymbolType symbolType) const;

   std::vector<const Record*> findWhereUsed(Record* record) const;

   std::vector<const Record*> findOverloadDefinitions(Record* record) const;

   Record* getDefinition(Record* record) const;
   Record* getDeclaration(Record* record) const;

   Record* followLocation(const Record::Location& location) const;
   Record* followLocation(const Record::Location& location, int endLine, int endColumn) const;

   std::vector<const Record*> dumpTranslationUnit(const std::string& fileName) const;

   std::vector<Record*> getBaseClasses(Record* record) const;
   std::vector<Record*> getDerivedClasses(Record* record) const;

   std::vector<char*> findFile(const std::string& fileNameFragment);
   std::vector<char*> findFileByRegex(const std::string& fileNamePattern);
   std::vector<char*> findFileUsingFuzzyMatch(const std::string& fuzzyInput);

   /*
    * Management
    */

   /** Merge the tags from other database into this one
    * @param other is the other database
    */
   void mergeFrom(const ProjectDb& other);

   void updateFrom(const std::string& fileName, const ProjectDb& other);

   /*
    * Debugging
    */
   void dumpRecords(std::ostream& os) const;

   void dumpStats(std::ostream& os) const;

   std::size_t getRecordCount() const
   {
      return m_recordSpanCache.getRecordCount();
   }

   std::vector<std::string> getStatisticsRemarks() const;

   bool isValid() const;

   std::size_t getTranslationUnitCount() const
   {
      return m_translationUnits.size();
   }

   std::size_t getSymbolCount() const
   {
      return m_symbolTable.getSize();
   }

   std::size_t getFilesCount() const
   {
      return m_fileNameTable.getSize();
   }

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static ftags::ProjectDb deserialize(ftags::BufferExtractor& extractor);

   /** Contains all the symbols in a C++ translation unit.
    */
   class TranslationUnit
   {
   public:
      using Key = ftags::StringTable::Key;

      void copyRecords(const TranslationUnit& other, RecordSpanCache& recordSpanCache);

      void copyRecords(const TranslationUnit& other,
                       RecordSpanCache&       recordSpanCache,
                       const KeyMap&          symbolKeyMapping,
                       const KeyMap&          fileNameKeyMapping);

      TranslationUnit(Key fileNameKey = 0) : m_fileNameKey{fileNameKey}
      {
      }

      Key getFileNameKey() const
      {
         return m_fileNameKey;
      }

      /*
       * Statistics
       */
      std::vector<Record>::size_type getRecordCount() const
      {
         return std::accumulate(m_recordSpans.cbegin(),
                                m_recordSpans.cend(),
                                0u,
                                [](std::vector<Record>::size_type acc, const std::shared_ptr<RecordSpan>& elem) {
                                   return acc + elem->getSize();
                                });
      }

      /*
       * General queries
       */
      std::vector<const Record*> getRecords(bool isFromMainFile) const
      {
         std::vector<const ftags::Record*> records;

         forEachRecord([&records, isFromMainFile](const ftags::Record* record) {
            if (record->attributes.isFromMainFile == isFromMainFile)
            {
               records.push_back(record);
            }
         });

         return records;
      }

      std::vector<Record*> getFunctions() const;

      std::vector<Record*> getClasses() const;

      std::vector<Record*> getGlobalVariables() const;

      /*
       * Specific queries
       */
      std::vector<Record*> findDeclaration(const std::string& symbolName) const;

      std::vector<Record*> findDeclaration(const std::string& symbolName, SymbolType type) const;

      std::vector<Record*> findDefinition(const std::string& symbolName) const;

      static TranslationUnit parse(const std::string&              fileName,
                                   const std::vector<const char*>& arguments,
                                   StringTable&                    symbolTable,
                                   StringTable&                    fileNameTable,
                                   ftags::RecordSpanCache&         recordSpanCache,
                                   const std::string&              filterPath);

      void addCursor(const Cursor&           cursor,
                     StringTable::Key        symbolNameKey,
                     StringTable::Key        fileNameKey,
                     StringTable::Key        referencedFileNameKey,
                     ftags::RecordSpanCache& spanCache);

      /*
       * Query helper
       */
      template <typename F>
      void forEachRecord(F func) const
      {
         std::for_each(m_recordSpans.cbegin(), m_recordSpans.cend(), [func](const std::shared_ptr<RecordSpan>& elem) {
            elem->forEachRecord(func);
         });
      }

      struct RecordSymbolComparator
      {
         RecordSymbolComparator(const Record* records) : m_records{records}
         {
         }

         bool operator()(std::vector<Record>::size_type recordPos, Key symbolNameKey)
         {
            return m_records[recordPos].symbolNameKey < symbolNameKey;
         }

         bool operator()(Key symbolNameKey, std::vector<Record>::size_type recordPos)
         {
            return symbolNameKey < m_records[recordPos].symbolNameKey;
         }

         const Record* m_records;
      };

      template <typename F>
      void forEachRecordWithSymbol(Key symbolNameKey, F func) const
      {
         std::for_each(m_recordSpans.cbegin(),
                       m_recordSpans.cend(),
                       [symbolNameKey, func](const std::shared_ptr<RecordSpan>& elem) {
                          elem->forEachRecordWithSymbol(symbolNameKey, func);
                       });
      }

      /*
       * Debugging
       */
      void dumpRecords(std::ostream&             os,
                       const ftags::StringTable& symbolTable,
                       const ftags::StringTable& fileNameTable) const;

      bool isValid() const;

      /*
       * Serialization interface
       */
      std::size_t computeSerializedSize() const;

      void serialize(ftags::BufferInsertor& insertor) const;

      static TranslationUnit deserialize(ftags::BufferExtractor& extractor, const RecordSpanCache& spanCache);

   private:
      // key of the file name of the main translation unit
      Key m_fileNameKey = 0;

      // persistent data
      std::vector<std::shared_ptr<RecordSpan>> m_recordSpans;

      Key m_currentRecordSpanFileKey = 0;

      std::vector<Record> m_currentSpan;
      void                flushCurrentSpan(RecordSpanCache& recordSpanCache);

      void beginParsingUnit(StringTable::Key fileNameKey);
      void finalizeParsingUnit(RecordSpanCache& recordSpanCache);
   };

   const TranslationUnit&
   parseOneFile(const std::string& fileName, std::vector<const char*> arguments, bool includeEverything = true);

private:
   const TranslationUnit& addTranslationUnit(const std::string& fileName, const TranslationUnit& translationUnit);

   void updateIndices();

   template <typename F>
   std::vector<const Record*> filterRecordsWithSymbol(const std::string& symbolName, F selectRecord) const
   {
      std::vector<const ftags::Record*> results;

      const auto key = m_symbolTable.getKey(symbolName.data());
      if (key)
      {
         const auto range = m_recordSpanCache.getSpansForSymbol(key);
         for (auto iter = range.first; iter != range.second; ++iter)
         {
            std::shared_ptr<RecordSpan> recordSpan = iter->second.lock();

            if (recordSpan)
            {
               recordSpan->forEachRecordWithSymbol(key, [&results, selectRecord](const Record* record) {
                  if (selectRecord(record))
                  {
                     results.push_back(record);
                  }
               });
            }
         }
      }

      return results;
   }

   std::string m_name;
   std::string m_root;

   using TranslationUnitStore = std::vector<TranslationUnit>;
   TranslationUnitStore m_translationUnits;

   StringTable m_symbolTable;
   StringTable m_namespaceTable;
   StringTable m_fileNameTable;

   /** Maps from a file name key to a position in the translation units vector.
    */
   std::map<StringTable::Key, std::vector<TranslationUnit>::size_type> m_fileIndex;

   RecordSpanCache m_recordSpanCache;
};

void parseProject(const char* parentDirectory, ftags::ProjectDb& projectDb);

} // namespace ftags

namespace std
{
template <>
struct hash<ftags::RecordSpan>
{
   size_t operator()(const ftags::RecordSpan& rs) const
   {
      return rs.getHash();
   }
};

} // namespace std

#endif // DB_PROJECT_H_INCLUDED
