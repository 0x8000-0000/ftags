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

#include <project.h>

#include <algorithm>
#include <numeric>
#include <ostream>

namespace
{

class OrderRecordsByLocation
{
public:
   OrderRecordsByLocation(const std::vector<ftags::Record>& records) : m_records{records}
   {
   }

   bool operator()(const unsigned& left, const unsigned& right) const
   {
      assert(right < m_records.size());
      assert(left < m_records.size());

      const ftags::Record& leftRecord  = m_records[left];
      const ftags::Record& rightRecord = m_records[right];

      if (leftRecord.fileNameKey < rightRecord.fileNameKey)
      {
         return true;
      }

      if (leftRecord.fileNameKey == rightRecord.fileNameKey)
      {
         if (leftRecord.startLine < rightRecord.startLine)
         {
            return true;
         }

         if (leftRecord.startLine == rightRecord.startLine)
         {
            if (leftRecord.startColumn < rightRecord.startColumn)
            {
               return true;
            }
         }
      }

      return false;
   }

private:
   const std::vector<ftags::Record>& m_records;
};

std::string getRecordType(ftags::Attributes attributes)
{
   const ftags::SymbolType symbolType = static_cast<ftags::SymbolType>(attributes.type);

   switch (symbolType)
   {
   case ftags::SymbolType::Undefined:
      return "Undefined";

   case ftags::SymbolType::StructDeclaration:
      return "StructDeclaration";

   case ftags::SymbolType::UnionDeclaration:
      return "UnionDeclaration";

   case ftags::SymbolType::ClassDeclaration:
      return "ClassDeclaration";

   case ftags::SymbolType::EnumerationDeclaration:
      return "EnumerationDeclaration";

   case ftags::SymbolType::FieldDeclaration:
      return "FieldDeclaration";

   case ftags::SymbolType::EnumerationConstantDeclaration:
      return "EnumerationConstantDeclaration";

   case ftags::SymbolType::FunctionDeclaration:
      return "FunctionDeclaration";

   case ftags::SymbolType::VariableDeclaration:
      return "VariableDeclaration";

   case ftags::SymbolType::ParameterDeclaration:
      return "ParameterDeclaration";

   case ftags::SymbolType::TypedefDeclaration:
      return "TypedefDeclaration";

   case ftags::SymbolType::MethodDeclaration:
      return "MethodDeclaration";

   case ftags::SymbolType::Namespace:
      return "Namespace";

   case ftags::SymbolType::Constructor:
      return "Constructor";

   case ftags::SymbolType::Destructor:
      return "Destructor";

   case ftags::SymbolType::ConversionFunction:
      return "ConversionFunction";

   case ftags::SymbolType::TemplateTypeParameter:
      return "TemplateTypeParameter";

   case ftags::SymbolType::NonTypeTemplateParameter:
      return "NonTypeTemplateParameter";

   case ftags::SymbolType::TemplateTemplateParameter:
      return "TemplateTemplateParameter";

   case ftags::SymbolType::FunctionTemplate:
      return "FunctionTemplate";

   case ftags::SymbolType::ClassTemplate:
      return "ClassTemplate";

   case ftags::SymbolType::ClassTemplatePartialSpecialization:
      return "ClassTemplatePartialSpecialization";

   case ftags::SymbolType::NamespaceAlias:
      return "NamespaceAlias";

   case ftags::SymbolType::UsingDirective:
      return "UsingDirective";

   case ftags::SymbolType::UsingDeclaration:
      return "UsingDeclaration";

   case ftags::SymbolType::TypeAliasDeclaration:
      return "TypeAliasDeclaration";

   case ftags::SymbolType::AccessSpecifier:
      return "AccessSpecifier";

   case ftags::SymbolType::TypeReference:
      return "TypeReference";

   case ftags::SymbolType::BaseSpecifier:
      return "BaseSpecifier";

   case ftags::SymbolType::TemplateReference:
      return "TemplateReference";

   case ftags::SymbolType::NamespaceReference:
      return "NamespaceReference";

   case ftags::SymbolType::MemberReference:
      return "MemberReference";

   case ftags::SymbolType::LabelReference:
      return "LabelReference";

   case ftags::SymbolType::OverloadedDeclarationReference:
      return "OverloadedDeclarationReference";

   case ftags::SymbolType::VariableReference:
      return "VariableReference";

   case ftags::SymbolType::DeclarationReferenceExpression:
      return "DeclarationReferenceExpression";

   case ftags::SymbolType::MemberReferenceExpression:
      return "MemberReferenceExpression";

   case ftags::SymbolType::FunctionCallExpression:
      return "FunctionCallExpression";

   case ftags::SymbolType::BlockExpression:
      return "BlockExpression";

   case ftags::SymbolType::IntegerLiteral:
      return "IntegerLiteral";

   case ftags::SymbolType::FloatingLiteral:
      return "FloatingLiteral";

   case ftags::SymbolType::ImaginaryLiteral:
      return "ImaginaryLiteral";

   case ftags::SymbolType::StringLiteral:
      return "StringLiteral";

   case ftags::SymbolType::CharacterLiteral:
      return "CharacterLiteral";

   case ftags::SymbolType::ArraySubscriptExpression:
      return "ArraySubscriptExpression";

   case ftags::SymbolType::CStyleCastExpression:
      return "CStyleCastExpression";

   case ftags::SymbolType::InitializationListExpression:
      return "InitializationListExpression";

   case ftags::SymbolType::StaticCastExpression:
   case ftags::SymbolType::DynamicCastExpression:
   case ftags::SymbolType::ReinterpretCastExpression:
   case ftags::SymbolType::ConstCastExpression:
   case ftags::SymbolType::FunctionalCastExpression:

   case ftags::SymbolType::TypeidExpression:
   case ftags::SymbolType::BoolLiteralExpression:
   case ftags::SymbolType::NullPtrLiteralExpression:
   case ftags::SymbolType::ThisExpression:
   case ftags::SymbolType::ThrowExpression:

   case ftags::SymbolType::NewExpression:
   case ftags::SymbolType::DeleteExpression:

   case ftags::SymbolType::LambdaExpression:
   case ftags::SymbolType::FixedPointLiteral:

   case ftags::SymbolType::MacroDefinition:
   case ftags::SymbolType::MacroExpansion:
   case ftags::SymbolType::InclusionDirective:

   case ftags::SymbolType::TypeAliasTemplateDecl:

      return "Something";

   default:
      return "Unknown";
   }
}

} // anonymous namespace

void ftags::RecordSpan::dumpRecords(std::ostream& os) const
{
   std::for_each(m_records.cbegin(), m_records.cend(), [&os, this](const Record& record) {
      const char* symbolName = getSymbolName(record);
      const char* fileName   = getFileName(record);
      std::string symbolType{getRecordType(record.attributes)};
      os << "    " << symbolName << "    " << symbolType << "   " << fileName << ':' << record.startLine << ':'
         << record.startColumn << std::endl;
   });
}

void ftags::TranslationUnit::dumpRecords(std::ostream& os) const
{
   os << " Found " << getRecordCount() << " records." << std::endl;

   std::for_each(m_recordSpans.cbegin(), m_recordSpans.cend(), [&os](const RecordSpan& rs) { rs.dumpRecords(os); });
}

void ftags::ProjectDb::dumpRecords(std::ostream& os) const
{
   std::for_each(
      m_translationUnits.cbegin(), m_translationUnits.cend(), [&os, this](const TranslationUnit& translationUnit) {
         const auto  fileNameKey = translationUnit.getFileNameKey();
         const char* fileName    = m_fileNameTable.getString(fileNameKey);
         if (fileName)
         {
            os << "File: " << fileName << std::endl;
         }
         else
         {
            os << "File: unnamed" << std::endl;
         }
         translationUnit.dumpRecords(os);
      });
}
