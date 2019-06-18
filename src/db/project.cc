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

      newRecord.symbolNameKey = m_symbolTable.addKey(original.getSymbolName(record));
      newRecord.fileNameKey   = m_fileNameTable.addKey(original.getFileName(record));

      m_records.push_back(newRecord);
   }
}

void ftags::TranslationUnit::addCursor(const ftags::Cursor& cursor, const ftags::Attributes& attributes)
{
   ftags::Record newRecord = {};

   newRecord.symbolNameKey = m_symbolTable.addKey(cursor.symbolName);
   newRecord.fileNameKey   = m_fileNameTable.addKey(cursor.location.fileName);

   newRecord.attributes      = attributes;
   newRecord.attributes.type = static_cast<uint32_t>(cursor.symbolType);

   newRecord.startLine   = static_cast<uint32_t>(cursor.location.line);
   newRecord.startColumn = static_cast<uint16_t>(cursor.location.column);
   newRecord.endLine     = static_cast<uint16_t>(cursor.endLine);

   m_records.push_back(newRecord);
}

void ftags::ProjectDb::addTranslationUnit(const std::string& fullPath, const TranslationUnit& translationUnit)
{
   /*
    * clone the records using the project's symbol table
    */
   TranslationUnit newTranslationUnit{m_symbolTable, m_fileNameTable};
   newTranslationUnit.copyRecords(translationUnit);

   /*
    * add the new translation unit to database
    */
   const TranslationUnitStore::size_type translationUnitPos = m_translationUnits.size();
   m_translationUnits.push_back(newTranslationUnit);

   /*
    * register the name of the translation unit
    */
   const auto fileKey   = m_fileNameTable.addKey(fullPath.data());
   m_fileIndex[fileKey] = translationUnitPos;

   /*
    * gather all unique symbols in this translation unit
    */
   std::set<ftags::StringTable::Key> symbolKeys;
   newTranslationUnit.forEachRecord(
      [&symbolKeys](const Record* record) { symbolKeys.insert(record->symbolNameKey); });

   /*
    * add a mapping from this symbol to this translation unit
    */
   std::for_each(
      symbolKeys.cbegin(), symbolKeys.cend(), [this, translationUnitPos](ftags::StringTable::Key symbolKey) {
         m_symbolIndex.emplace(symbolKey, translationUnitPos);
      });

   // TODO: save the Record's source file into the file name table
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
      translationUnit.forEachRecord([&functions](const Record* record) {
         if (record->attributes.type == static_cast<uint32_t>(SymbolType::FunctionDeclaration))
         {
            functions.push_back(record);
         }
      });
   }

   return functions;
}

std::vector<const ftags::Record*> ftags::ProjectDb::findDefinition(const std::string& symbolName) const
{
   std::vector<const ftags::Record*> results;

   const auto key = m_symbolTable.getKey(symbolName.data());
   if (key)
   {
      const auto keyRange = m_symbolIndex.equal_range(key);
      for (auto iter = keyRange.first; iter != keyRange.second; ++iter)
      {
         const auto& translationUnit = m_translationUnits.at(iter->second);

         translationUnit.forEachRecord([&results, key](const Record* record) {
            if (record->attributes.isDefinition && (record->symbolNameKey == key))
            {
               results.push_back(record);
            }
         });
      }
   }

   return results;
}

std::vector<const ftags::Record*> ftags::ProjectDb::findDeclaration(const std::string& symbolName) const
{
   std::vector<const ftags::Record*> results;

   const auto key = m_symbolTable.getKey(symbolName.data());
   if (key)
   {
      const auto range = m_symbolIndex.equal_range(key);
      for (auto translationUnitPos = range.first; translationUnitPos != range.second; ++translationUnitPos)
      {
         const auto& translationUnit = m_translationUnits.at(translationUnitPos->second);

         translationUnit.forEachRecord([&results, key](const Record* record) {
            if ((!record->attributes.isDefinition) && (record->symbolNameKey == key))
            {
               results.push_back(record);
            }
         });
      }
   }

   return results;
}
