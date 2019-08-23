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

#ifndef FTAGS_DB_RECORD_H_INCLUDED
#define FTAGS_DB_RECORD_H_INCLUDED

#include <string_table.h>

#include <string>

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

struct Record
{
   using Store = ftags::Store<Record, uint32_t, 24>;
   using Key   = Store::key_type;

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

} // namespace ftags

#endif // FTAGS_DB_RECORD_H_INCLUDED
