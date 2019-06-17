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

ftags::ProjectDb::ProjectDb() : m_operatingState{OptimizedForParse}
{
}

void ftags::TranslationUnit::copyRecords(const TranslationUnit& original)
{
   /*
    * iterate over the original records and copy them locally
    */
   for (const auto& record : original.m_records)
   {
      Record newRecord = record;

      newRecord.symbolNameKey = m_symbolTable.addKey(original.m_symbolTable.getString(record.symbolNameKey));
      newRecord.fileNameKey = m_fileNameTable.addKey(original.m_fileNameTable.getString(record.fileNameKey));

      m_records.push_back(newRecord);
   }
}

void ftags::TranslationUnit::addCursor(const ftags::Cursor& cursor, const ftags::Attributes& attributes)
{
   ftags::Record newRecord = {};

   newRecord.symbolNameKey   = m_symbolTable.addKey(cursor.symbolName);

   newRecord.attributes      = attributes;
   newRecord.attributes.type = static_cast<uint32_t>(cursor.symbolType);

   newRecord.startLine       = static_cast<uint32_t>(cursor.location.line);
   newRecord.startColumn     = static_cast<uint16_t>(cursor.location.column);
   newRecord.endLine         = static_cast<uint16_t>(cursor.endLine);

   m_records.push_back(newRecord);
}

void ftags::TranslationUnit::appendFunctionRecords(std::vector<const ftags::Record*>& records) const
{
   for (const auto& record : m_records)
   {
      if (record.attributes.type == static_cast<uint32_t>(SymbolType::FunctionDeclaration))
      {
         records.push_back(&record);
      }
   }
}

void ftags::ProjectDb::addTranslationUnit(const std::string& fullPath, const TranslationUnit& translationUnit)
{
   const auto key = m_fileNameTable.addKey(fullPath.data());

   TranslationUnit newTranslationUnit{m_symbolTable, m_fileNameTable};
   newTranslationUnit.copyRecords(translationUnit);
   m_translationUnits.push_back(newTranslationUnit);
   m_fileIndex[key] = m_translationUnits.size();
}

bool ftags::ProjectDb::isFileIndexed(const std::string& fileName) const
{
   bool       isIndexed = false;
   const auto key       = m_fileNameTable.getKey(fileName.data());
   if (key)
   {
      isIndexed = m_fileIndex.count(key) != 0;
   }
   return isIndexed;
}

std::vector<const ftags::Record*> ftags::ProjectDb::getFunctions() const
{
   std::vector<const ftags::Record*> functions;

   for (const auto& translationUnit : m_translationUnits)
   {
      translationUnit.appendFunctionRecords(functions);
   }

   return functions;
}
