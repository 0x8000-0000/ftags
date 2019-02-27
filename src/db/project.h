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

#include <plf_colony.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <cstdint>

namespace ftags
{

/*
 * Maps to Clang-C interface numeric values
 */
enum class SymbolType : uint8_t
{
   StructDeclaration      = 2,
   UnionDeclaration       = 3,
   ClassDeclaration       = 4,
   EnumerationDeclaration = 5,

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

   NamespaceAlias   = 33,
   UsingDirective   = 34,
   UsingDeclaration = 35,
   TypeAliasDecl    = 36,
   AccessSpecifier  = 39,

   TypeReference      = 43,
   BaseSpecifier      = 44,
   TemplateReference  = 45,
   NamespaceReference = 46,
   MemberReference    = 47,
   LabelReference     = 48,

   OverloadedDeclarationReference = 49,
   VariableReference              = 50,

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

   uint32_t type : 8;

   uint32_t isDeclaration : 1;
   uint32_t isDefinition : 1;
   uint32_t isUse : 1;
   uint32_t isOverload : 1;

   uint32_t isArray : 1;
   uint32_t isConstant : 1;
   uint32_t isGlobal : 1;
   uint32_t isMember : 1;

   uint32_t isCast : 1;
   uint32_t isParameter : 1;
   uint32_t isConstructed : 1;
   uint32_t isDestructed : 1;

   uint32_t isThrown : 1;

   // 3 free bits
};

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

   SymbolType symbolType;

   Location location;
   int      endLine;
   int      endColumn;
};

struct Record
{
   uint32_t symbolNameKey;
   uint32_t namespaceKey;

   uint32_t fileNameKey;

   uint32_t startLine;
   uint16_t startColumn;
   uint16_t endLine;

   uint64_t parentRecord;

   Attributes attributes;
};

class ProjectDb
{
public:
   /*
    * Construction
    */

   ProjectDb();

   Record* addCursor(const Cursor& cursor, const Attributes& attributes);

   void eraseRecords(const std::string& fileName);

   Cursor inflateRecord(const Record* record);

   static ProjectDb parseTranslationUnit(const std::string& fileName, std::vector<const char*> arguments);

   /*
    * General queries
    */
   std::vector<Record*> getFunctions() const;

   std::vector<Record*> getClasses() const;

   std::vector<Record*> getGlobalVariables() const;

   bool isFileIndexed(const std::string& fileName) const;

   /*
    * Specific queries
    */

   std::vector<Record*> findDeclaration(const std::string& symbolName) const;

   std::vector<Record*> findDeclaration(const std::string& symbolName, SymbolType type) const;

   std::vector<Record*> findDefinition(const std::string& symbolName) const;

   std::vector<Record*> findWhereUsed(Record* record) const;

   std::vector<Record*> findOverloadDefinitions(Record* record) const;

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

private:
   enum State
   {
      OptimizedForParse,
      OptimizedForQuery,
   } m_operatingState;

   void updateIndices();

   plf::colony<Record> m_records;

   StringTable m_symbolTable;
   StringTable m_namespaceTable;
   StringTable m_fileNameTable;

   std::map<uint32_t, std::vector<plf::colony<Record>::iterator>> m_fileIndex;

   std::map<uint32_t, std::vector<plf::colony<Record>::iterator>> m_symbolIndex;
};

} // namespace ftags

#endif // DB_PROJECT_H_INCLUDED
