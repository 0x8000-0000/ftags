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

#include <string_table.h>
#include <serialization.h>

#include <algorithm>
#include <iosfwd>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
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
   uint32_t lineSpan : 8;

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

   uint32_t isNamespaceRef : 1;

   uint32_t freeBits : 15;
};

static_assert(sizeof(Attributes) == 8, "sizeof(Attributes) exceeds 4 bytes");

struct Location
{
   const char* fileName;
   int         line;
   int         column;
};

struct Cursor
{
   char*       symbolNamespace;
   const char* symbolName;
   const char* unifiedSymbol;

   SymbolType symbolType;

   Location location;
   int      endLine;
   int      endColumn;
};

struct Record
{
   using SymbolNameKey    = ftags::StringTable::Key;
   using NamespaceNameKey = ftags::StringTable::Key;
   using FileNameKey      = ftags::StringTable::Key;

   SymbolNameKey    symbolNameKey;
   NamespaceNameKey namespaceKey;

   FileNameKey fileNameKey;
   uint32_t    startLine;
   uint16_t    startColumn;
   uint16_t    endLine;

   Attributes attributes;

   SymbolType getType() const
   {
      return attributes.type;
   }
};

static_assert(sizeof(Record) == 28, "sizeof(Record) exceeds 28 bytes");

class TranslationUnit
{
public:
   using Key = ftags::StringTable::Key;

   TranslationUnit(StringTable& symbolTable, StringTable& fileNameTable) :
      m_symbolTable{symbolTable},
      m_fileNameTable{fileNameTable}
   {
   }

   void copyRecords(const TranslationUnit& other);

   Key getFileNameKey() const
   {
      return m_fileNameKey;
   }

   const char* getSymbolName(const Record& record) const
   {
      return m_symbolTable.getString(record.symbolNameKey);
   }

   const char* getFileName(const Record& record) const
   {
      return m_fileNameTable.getString(record.fileNameKey);
   }

   /*
    * Statistics
    */
   std::vector<Record>::size_type getRecordCount() const
   {
      return m_records.size();
   }

   /*
    * General queries
    */
   std::vector<Record*> getFunctions() const;

   std::vector<Record*> getClasses() const;

   std::vector<Record*> getGlobalVariables() const;

   /*
    * Specific queries
    */
   std::vector<Record*> findDeclaration(const std::string& symbolName) const;

   std::vector<Record*> findDeclaration(const std::string& symbolName, SymbolType type) const;

   std::vector<Record*> findDefinition(const std::string& symbolName) const;

   static TranslationUnit parse(const std::string&       fileName,
                                std::vector<const char*> arguments,
                                StringTable&             symbolTable,
                                StringTable&             fileNameTable);

   void addCursor(const Cursor& cursor, const Attributes& attributes);

   /*
    * Query helper
    */
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

   /*
    * Debugging
    */
   void dumpRecords(std::ostream& os) const;

private:
   // key of the file name of the main translation unit
   Key m_fileNameKey = 0;

   // persistent data
   std::vector<Record> m_records;

   // tables managed by the owner of this translation unit
   StringTable& m_symbolTable;
   StringTable& m_fileNameTable;

   void updateIndices();

   // indexes of records based of different traversal order
   std::vector<std::vector<Record>::size_type> m_recordsInSymbolKeyOrder;
   std::vector<std::vector<Record>::size_type> m_recordsInFileKeyOrder;
};

class CursorSet
{
public:
   CursorSet(std::vector<const Record*> records, const StringTable& symbolTable, const StringTable& fileNameTable);

   Cursor inflateRecord(const Record& record);

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

private:

   CursorSet();

   // persistent data
   std::vector<Record> m_records;
   StringTable         m_symbolTable;
   StringTable         m_fileNameTable;
};

class ProjectDb
{
public:
   /*
    * Construction and maintenance
    */
   ProjectDb();

   void addTranslationUnit(const std::string& fileName, const TranslationUnit& translationUnit);

   void removeTranslationUnit(const std::string& fileName);

   Cursor inflateRecord(const Record* record);

   CursorSet inflateRecords(const std::vector<const Record*>& records);

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

   Record* followLocation(const Location& location) const;
   Record* followLocation(const Location& location, int endLine, int endColumn) const;

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

   /*
    * Serialization
    */

   std::vector<uint8_t> serialize() const;

   static ProjectDb deserialize(const uint8_t* buffer, size_t size);

   /*
    * Debugging
    */
   void dumpRecords(std::ostream& os) const;

private:
   enum State
   {
      OptimizedForParse,
      OptimizedForQuery,
   } m_operatingState;

   void updateIndices();

   template <typename F>
   std::vector<const Record*> filterRecordsWithSymbol(const std::string& symbolName, F selectRecord) const
   {
      std::vector<const ftags::Record*> results;

      const auto key = m_symbolTable.getKey(symbolName.data());
      if (key)
      {
         const auto range = m_symbolIndex.equal_range(key);
         for (auto translationUnitPos = range.first; translationUnitPos != range.second; ++translationUnitPos)
         {
            const auto& translationUnit = m_translationUnits.at(translationUnitPos->second);

            translationUnit.forEachRecordWithSymbol(key, [&results, selectRecord](const Record* record) {
               if (selectRecord(record))
               {
                  results.push_back(record);
               }
            });
         }
      }

      return results;
   }

   /** Contains all the symbol definitions.
    */
   using TranslationUnitStore = std::vector<TranslationUnit>;
   TranslationUnitStore m_translationUnits;
   std::mutex           m_updateTranslationUnits;

   StringTable m_symbolTable{true}; // enable concurrent access on all symbol tables
   StringTable m_namespaceTable{true};
   StringTable m_fileNameTable{true};

   /** Maps from a symbol key to a bag of translation units containing the symbol.
    */
   std::multimap<StringTable::Key, TranslationUnitStore::size_type> m_symbolIndex;

   /** Maps from a file name key to a position in the translation units vector.
    */
   std::map<StringTable::Key, std::vector<TranslationUnit>::size_type> m_fileIndex;
};

void parseProject(const char* parentDirectory, ftags::ProjectDb& projectDb);

} // namespace ftags

#endif // DB_PROJECT_H_INCLUDED
