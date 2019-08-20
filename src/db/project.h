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

/*
 * Maps to Clang-C interface numeric values
 */
enum class SymbolType : uint16_t
{
   Undefined = 0,

   StructDeclaration              = 2,
   UnionDeclaration               = 3,
   ClassDeclaration               = 4,
   EnumerationDeclaration         = 5,
   FieldDeclaration               = 6,
   EnumerationConstantDeclaration = 7,
   FunctionDeclaration            = 8,
   VariableDeclaration            = 9,
   ParameterDeclaration           = 10,

   TypedefDeclaration = 20,
   MethodDeclaration  = 21,
   Namespace          = 22,

   Constructor        = 24,
   Destructor         = 25,
   ConversionFunction = 26,

   TemplateTypeParameter              = 27,
   NonTypeTemplateParameter           = 28,
   TemplateTemplateParameter          = 29,
   FunctionTemplate                   = 30,
   ClassTemplate                      = 31,
   ClassTemplatePartialSpecialization = 32,

   NamespaceAlias       = 33,
   UsingDirective       = 34,
   UsingDeclaration     = 35,
   TypeAliasDeclaration = 36,
   AccessSpecifier      = 39,

   TypeReference      = 43,
   BaseSpecifier      = 44,
   TemplateReference  = 45,
   NamespaceReference = 46,
   MemberReference    = 47,
   LabelReference     = 48,

   OverloadedDeclarationReference = 49,
   VariableReference              = 50,

   UnexposedExpression            = 100,
   DeclarationReferenceExpression = 101,
   MemberReferenceExpression      = 102,
   FunctionCallExpression         = 103,

   BlockExpression = 105,

   IntegerLiteral   = 106,
   FloatingLiteral  = 107,
   ImaginaryLiteral = 108,
   StringLiteral    = 109,
   CharacterLiteral = 110,

   ArraySubscriptExpression = 113,

   CStyleCastExpression = 117,

   InitializationListExpression = 119,

   StaticCastExpression      = 124,
   DynamicCastExpression     = 125,
   ReinterpretCastExpression = 126,
   ConstCastExpression       = 127,
   FunctionalCastExpression  = 128,

   TypeidExpression         = 129,
   BoolLiteralExpression    = 130,
   NullPtrLiteralExpression = 131,
   ThisExpression           = 132,
   ThrowExpression          = 133,

   NewExpression    = 134,
   DeleteExpression = 135,

   LambdaExpression  = 144,
   FixedPointLiteral = 149,

   MacroDefinition    = 501,
   MacroExpansion     = 502,
   InclusionDirective = 503,

   TypeAliasTemplateDecl = 601,
};

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

struct Attributes
{
   SymbolType type : 16;

   uint32_t isDeclaration : 1;
   uint32_t isDefinition : 1;
   uint32_t isUse : 1;
   uint32_t isOverload : 1;

   uint32_t isReference : 1;
   uint32_t isExpression : 1;

   uint32_t isArray : 1;
   uint32_t isConstant : 1;
   uint32_t isGlobal : 1;
   uint32_t isMember : 1;

   uint32_t isCast : 1;
   uint32_t isParameter : 1;
   uint32_t isConstructed : 1;
   uint32_t isDestructed : 1;

   uint32_t isThrown : 1;

   uint32_t isFromMainFile : 1;
   uint32_t isDefinedInMainFile : 1;

   uint32_t isNamespaceRef : 1;

   uint32_t freeBits : 22;

   std::string getRecordType() const;

   std::string getRecordFlavor() const;
};

static_assert(sizeof(Attributes) == 8, "sizeof(Attributes) exceeds 8 bytes");

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

struct Record
{
   using SymbolNameKey    = ftags::StringTable::Key;
   using NamespaceNameKey = ftags::StringTable::Key;
   using FileNameKey      = ftags::StringTable::Key;

   struct Location
   {
      FileNameKey fileNameKey;
      uint32_t    line : 20;
      uint32_t    column : 12;
   };

   SymbolNameKey    symbolNameKey;
   NamespaceNameKey namespaceKey;

   Location location;
   Location definition;

   Attributes attributes;

   SymbolType getType() const
   {
      return attributes.type;
   }

   void setLocationFileKey(FileNameKey key)
   {
      location.fileNameKey = key;
   }

   void setDefinitionFileKey(FileNameKey key)
   {
      definition.fileNameKey = key;
   }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

   void setLocationAddress(unsigned int line, unsigned int column)
   {
      location.line   = static_cast<uint32_t>(line);
      location.column = static_cast<uint32_t>(column);
   }

   void setDefinitionAddress(unsigned int line, unsigned int column)
   {
      definition.line   = static_cast<uint32_t>(line);
      definition.column = static_cast<uint32_t>(column);
   }
#pragma GCC diagnostic pop
};

static_assert(sizeof(Record) == 32, "sizeof(Record) exceeds 32 bytes");

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
public:
   class HashingFunctor
   {
   public:
      size_t operator()(const std::weak_ptr<RecordSpan>& weakVal) const
      {
         std::shared_ptr<RecordSpan> val = weakVal.lock();
         if (!val)
         {
            return 0;
         }
         else
         {
            return val->getHash();
         }
      }
   };

   class CompareFunctor
   {
   public:
      bool operator()(const std::weak_ptr<RecordSpan>& weakLeftVal,
                      const std::weak_ptr<RecordSpan>& weakRightVal) const
      {
         std::shared_ptr<RecordSpan> leftVal  = weakLeftVal.lock();
         std::shared_ptr<RecordSpan> rightVal = weakRightVal.lock();

         if (!leftVal)
         {
            if (!rightVal)
            {
               return true;
            }
            else
            {
               return false;
            }
         }
         else
         {
            if (!rightVal)
            {
               return false;
            }
            else
            {
               return leftVal->operator==(*rightVal);
            }
         }
      }
   };

   using Key = ftags::StringTable::Key;

   Key getFileNameKey() const
   {
      return m_fileNameKey;
   }

   /*
    * Statistics
    */
   std::vector<Record>::size_type getRecordCount() const
   {
      return m_records.size();
   }

   void addRecord(const ftags::Record& record)
   {
      m_records.push_back(record);
   }

   Record& getLastRecord()
   {
      return m_records.back();
   }

   void addRecords(const RecordSpan& other);

   void addRecords(const RecordSpan&               other,
                   const ftags::FlatMap<Key, Key>& symbolKeyMapping,
                   const ftags::FlatMap<Key, Key>& fileNameKeyMapping);

   bool operator==(const RecordSpan& other) const
   {
      // return std::equal(m_records.cbegin(), m_records.cend(), other.m_records.cbegin());
      if (m_records.size() == other.m_records.size())
      {
         return 0 == memcmp(m_records.data(), other.m_records.data(), sizeof(Record) * m_records.size());
      }
      else
      {
         return false;
      }
   }

   template <typename F>
   void forEachRecord(F func) const
   {
      for (const auto& record : m_records)
      {
         func(&record);
      }
   }

   struct RecordSymbolComparator
   {
      RecordSymbolComparator(const std::vector<Record>& records) : m_records{records}
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

      const std::vector<Record>& m_records;
   };

   template <typename F>
   void forEachRecordWithSymbol(Key symbolNameKey, F func) const
   {
      const auto keyRange = std::equal_range(m_recordsInSymbolKeyOrder.cbegin(),
                                             m_recordsInSymbolKeyOrder.cend(),
                                             symbolNameKey,
                                             RecordSymbolComparator(m_records));

      for (auto iter = keyRange.first; iter != keyRange.second; ++iter)
      {
         func(&m_records[*iter]);
      }
   }

   void updateIndices();

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(ftags::BufferInsertor& insertor) const;

   static RecordSpan deserialize(ftags::BufferExtractor& extractor);

   /*
    * Debugging
    */
   void dumpRecords(std::ostream&             os,
                    const ftags::StringTable& symbolTable,
                    const ftags::StringTable& fileNameTable) const;

   using hash_type = std::size_t;

   hash_type getHash() const
   {
      return m_hash;
   }

private:
   // key of the file name of the main translation unit
   Key m_fileNameKey = 0;

   // persistent data
   std::vector<Record> m_records;

   // 128 bit hash of the record span
   std::size_t m_hash = 0;

   // reference counter
   int m_usageCount = 0;

   std::vector<std::vector<Record>::size_type> m_recordsInSymbolKeyOrder;

   static constexpr uint64_t k_hashSeed = 0x0accedd62cf0b9bf;
};

class RecordSpanCache
{
private:
   using cache_type = std::unordered_multimap<RecordSpan::hash_type, std::weak_ptr<RecordSpan>>;

   using value_type = cache_type::value_type;

   using iterator_type = cache_type::iterator;

   using index_type = std::multimap<StringTable::Key, std::weak_ptr<RecordSpan>>;

   cache_type m_cache;

   std::size_t m_spansObserved = 0;

   /** Maps from a symbol key to a bag of translation units containing the symbol.
    */
   index_type m_symbolIndex;

   void indexRecordSpan(std::shared_ptr<ftags::RecordSpan> original);

public:
   std::pair<index_type::const_iterator, index_type::const_iterator> getSpansForSymbol(StringTable::Key key) const
   {
      return m_symbolIndex.equal_range(key);
   }

   std::shared_ptr<RecordSpan> add(std::shared_ptr<RecordSpan> newSpan);

   std::vector<std::shared_ptr<RecordSpan>> get(RecordSpan::hash_type spanHash) const
   {
      std::vector<std::shared_ptr<RecordSpan>> retval;

      std::pair<cache_type::const_iterator, cache_type::const_iterator> range = m_cache.equal_range(spanHash);

      for (auto iter = range.first; iter != range.second; ++iter)
      {
         retval.push_back(iter->second.lock());
      }

      return retval;
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

      void copyRecords(const TranslationUnit& other, RecordSpanCache& spanCache);

      void copyRecords(const TranslationUnit& other,
                       RecordSpanCache&       spanCache,
                       const KeyMap&          symbolKeyMapping,
                       const KeyMap&          fileNameKeyMapping);

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
                                   return acc + elem->getRecordCount();
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
                                   const std::string&              filterPath);

      void addCursor(const Cursor&    cursor,
                     StringTable::Key symbolNameKey,
                     StringTable::Key fileNameKey,
                     StringTable::Key referencedFileNameKey);

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
         RecordSymbolComparator(const std::vector<Record>& records) : m_records{records}
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

         const std::vector<Record>& m_records;
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

      void updateIndices();
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
