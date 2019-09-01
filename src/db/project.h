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
#include <record_span_manager.h>

#include <serialization.h>
#include <string_table.h>

#include <algorithm>
#include <filesystem>
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

using KeyMap = ftags::util::FlatMap<ftags::util::StringTable::Key, ftags::util::StringTable::Key>;

class CursorSet
{
public:
   CursorSet(std::vector<const Record*>      records,
             const ftags::util::StringTable& symbolTable,
             const ftags::util::StringTable& fileNameTable);

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

   void serialize(ftags::util::BufferInsertor& insertor) const;

   static CursorSet deserialize(ftags::util::BufferExtractor& extractor);

   std::size_t computeHash() const;

private:
   CursorSet() = default;

   // persistent data
   std::vector<Record>      m_records;
   ftags::util::StringTable m_symbolTable;
   ftags::util::StringTable m_fileNameTable;

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

   ProjectDb(const ProjectDb& other) = delete;
   const ProjectDb& operator=(const ProjectDb& other) = delete;

   ProjectDb(ProjectDb&& other) :
      m_name{std::move(other.m_name)},
      m_root{std::move(other.m_root)},
      m_translationUnits{std::move(other.m_translationUnits)},
      m_symbolTable{std::move(other.m_symbolTable)},
      m_namespaceTable{std::move(other.m_namespaceTable)},
      m_fileNameTable{std::move(other.m_fileNameTable)},
      m_recordSpanManager{std::move(other.m_recordSpanManager)},
      m_fileIndex{std::move(other.m_fileIndex)}
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

   std::vector<const Record*>
           identifySymbol(const std::string& fileName, unsigned lineNumber, unsigned columnNumber) const;
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
   void dumpRecords(std::ostream& os, const std::filesystem::path& trimPath) const;

   void dumpStats(std::ostream& os) const;

   std::size_t getRecordCount() const
   {
      return m_recordSpanManager.getRecordCount();
   }

   std::vector<std::string> getStatisticsRemarks(const std::string& statisticsGroup) const;

   void assertValid() const
#if defined(NDEBUG) || (!defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
   {
   }
#else
      ;
#endif

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

   void serialize(ftags::util::BufferInsertor& insertor) const;

   static ftags::ProjectDb deserialize(ftags::util::BufferExtractor& extractor);

   /** Contains all the symbols in a C++ translation unit.
    */
   class TranslationUnit
   {
   public:
      using Key = ftags::util::StringTable::Key;

      void copyRecords(const TranslationUnit&   other,
                       const RecordSpanManager& otherRecordSpanManager,
                       RecordSpanManager&       recordSpanManager);

      void copyRecords(const TranslationUnit&   other,
                       const RecordSpanManager& otherRecordSpanManager,
                       RecordSpanManager&       recordSpanManager,
                       const KeyMap&            symbolKeyMapping,
                       const KeyMap&            fileNameKeyMapping);

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
      std::vector<Record>::size_type getRecordCount(const RecordSpanManager& recordSpanManager) const
      {
         return std::accumulate(m_recordSpans.cbegin(),
                                m_recordSpans.cend(),
                                0u,
                                [&recordSpanManager](std::vector<Record>::size_type acc, RecordSpan::Store::Key key) {
                                   const RecordSpan& recordSpan = recordSpanManager.getSpan(key);
                                   return acc + recordSpan.getSize();
                                });
      }

      /*
       * General queries
       */
      std::vector<const Record*> getRecords(bool isFromMainFile, const RecordSpanManager& recordSpanManager) const
      {
         std::vector<const ftags::Record*> records;

         forEachRecord(
            [&records, isFromMainFile](const ftags::Record* record) {
               if (record->attributes.isFromMainFile == isFromMainFile)
               {
                  records.push_back(record);
               }
            },
            recordSpanManager);

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
                                   ftags::util::StringTable&       symbolTable,
                                   ftags::util::StringTable&       fileNameTable,
                                   RecordSpanManager&              recordSpanManager,
                                   const std::string&              filterPath);

      void addCursor(const Cursor&                 cursor,
                     ftags::util::StringTable::Key symbolNameKey,
                     ftags::util::StringTable::Key fileNameKey,
                     ftags::util::StringTable::Key referencedFileNameKey,
                     ftags::RecordSpanManager&     recordSpanManager);

      /*
       * Query helper
       */
      template <typename F>
      void forEachRecordSpan(F func, const RecordSpanManager& recordSpanManager) const
      {
         std::for_each(
            m_recordSpans.cbegin(), m_recordSpans.cend(), [func, &recordSpanManager](RecordSpan::Store::Key key) {
               const RecordSpan& recordSpan = recordSpanManager.getSpan(key);
               func(recordSpan);
            });
      }

      template <typename F>
      void forEachRecord(F func, const RecordSpanManager& recordSpanManager) const
      {
         std::for_each(
            m_recordSpans.cbegin(), m_recordSpans.cend(), [func, &recordSpanManager](RecordSpan::Store::Key key) {
               const RecordSpan& recordSpan = recordSpanManager.getSpan(key);
               recordSpan.forEachRecord(func);
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
      void dumpRecords(std::ostream&                   os,
                       const RecordSpanManager&        recordSpanManager,
                       const ftags::util::StringTable& symbolTable,
                       const ftags::util::StringTable& fileNameTable,
                       const std::filesystem::path&    trimPath) const;

      void assertValid() const
#if defined(NDEBUG) || (!defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
      {
      }
#else
         ;
#endif

      /*
       * Serialization interface
       */
      std::size_t computeSerializedSize() const;

      void serialize(ftags::util::BufferInsertor& insertor) const;

      static TranslationUnit deserialize(ftags::util::BufferExtractor& extractor);

   private:
      // key of the file name of the main translation unit
      Key m_fileNameKey = 0;

      // persistent data
      std::vector<RecordSpan::Store::Key> m_recordSpans;

      Key m_currentRecordSpanFileKey = 0;

      std::vector<Record> m_currentSpan;

      struct SymbolAtLocation
      {
         ftags::util::StringTable::Key symbolKey;
         Record::Location              location;

         SymbolAtLocation(ftags::util::StringTable::Key symbolKey_,
                          ftags::util::StringTable::Key fileKey,
                          uint32_t                      line,
                          uint32_t                      column) :
            symbolKey{symbolKey_},
            location{fileKey, line, column}
         {
         }

         bool operator==(const SymbolAtLocation& other) const
         {
            return (symbolKey == other.symbolKey) && (location == other.location);
         }

         bool operator<(const SymbolAtLocation& other) const
         {
            if (symbolKey < other.symbolKey)
            {
               return true;
            }
            if (symbolKey == other.symbolKey)
            {
               if (location < other.location)
               {
                  return true;
               }
            }
            return false;
         }
      };

      std::set<SymbolAtLocation> m_currentSpanLocations;
      void                       flushCurrentSpan(RecordSpanManager& recordSpanManager);

      void beginParsingUnit(ftags::util::StringTable::Key fileNameKey);
      void finalizeParsingUnit(RecordSpanManager& recordSpanManager);
   };

   const TranslationUnit&
   parseOneFile(const std::string& fileName, std::vector<const char*> arguments, bool includeEverything = true);

   std::vector<const Record*> getTranslationUnitRecords(const TranslationUnit& translationUnit,
                                                        bool                   isFromMainFile) const
   {
      return translationUnit.getRecords(isFromMainFile, m_recordSpanManager);
   }

private:
   const TranslationUnit& addTranslationUnit(const std::string&       fileName,
                                             const TranslationUnit&   translationUnit,
                                             const RecordSpanManager& otherRecordSpanManager);

   void updateIndices();

   template <typename F>
   std::vector<const Record*> filterRecordsWithSymbol(const std::string& symbolName, F selectRecord) const
   {
      std::vector<const ftags::Record*> results;

      const auto key = m_symbolTable.getKey(symbolName.data());
      if (key)
      {
         results = m_recordSpanManager.filterRecordsWithSymbol(key, selectRecord);
      }

      Record::filterDuplicates(results);

      return results;
   }

   template <typename F>
   std::vector<const Record*> filterRecordsFromFile(const std::string& fileName, F selectRecord) const
   {
      std::vector<const ftags::Record*> results;

      const auto key = m_fileNameTable.getKey(fileName.data());
      if (key)
      {
         results = m_recordSpanManager.filterRecordsFromFile(key, selectRecord);
      }

      return results;
   }

   std::string m_name;
   std::string m_root;

   using TranslationUnitStore = std::vector<TranslationUnit>;
   TranslationUnitStore m_translationUnits;

   ftags::util::StringTable m_symbolTable;
   ftags::util::StringTable m_namespaceTable;
   ftags::util::StringTable m_fileNameTable;

   RecordSpanManager m_recordSpanManager;

   /** Maps from a file name key to a position in the translation units vector.
    */
   std::map<ftags::util::StringTable::Key, std::vector<TranslationUnit>::size_type> m_fileIndex;
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
