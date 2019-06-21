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
    * copy the original records
    */
   m_records = original.m_records;

   /*
    * replace keys from translation unit's table with keys from the prject
    * database
    */

   // update file keys
   {
      optimizeForFileLookup();

      Key currentOriginalFileKey = 0;
      Key currentFileKey         = 0;

      for (auto& record : m_records)
      {
         if (record.fileNameKey != currentOriginalFileKey)
         {
            // TODO: lock for concurrent access
            currentFileKey = m_fileNameTable.addKey(original.getFileName(record));
         }
         record.fileNameKey = currentFileKey;
      }
   }

   // update symbol keys
   {
      optimizeForSymbolLookup();

      Key currentOriginalSymbolKey = 0;
      Key currentSymbolKey         = 0;

      for (auto& record : m_records)
      {
         if (record.symbolNameKey != currentOriginalSymbolKey)
         {
            // TODO: lock for concurrent access
            currentSymbolKey = m_symbolTable.addKey(original.getSymbolName(record));
         }
         record.symbolNameKey = currentSymbolKey;
      }
   }
}

void ftags::TranslationUnit::optimizeForSymbolLookup()
{
   /*
    * sort the records by symbol key to speed-up lookups
    */
   std::sort(m_records.begin(), m_records.end(), [](const Record& left, const Record& right) {
      return left.symbolNameKey < right.symbolNameKey;
   });
}

void ftags::TranslationUnit::optimizeForFileLookup()
{
   /*
    * sort the records by symbol key to speed-up lookups
    */
   std::sort(m_records.begin(), m_records.end(), [](const Record& left, const Record& right) {
      return left.fileNameKey < right.fileNameKey;
   });
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
    * gather all unique symbols in this translation unit
    */
   std::set<ftags::StringTable::Key> symbolKeys;
   newTranslationUnit.forEachRecord(
      [&symbolKeys](const Record* record) { symbolKeys.insert(record->symbolNameKey); });

   /*
    * protect access
    */
   std::lock_guard<std::mutex>           lock(m_updateTranslationUnits);

   /*
    * add the new translation unit to database
    */
   const TranslationUnitStore::size_type translationUnitPos = m_translationUnits.size();
   m_translationUnits.emplace_back(std::move(newTranslationUnit));

   /*
    * register the name of the translation unit
    */
   const auto fileKey   = m_fileNameTable.addKey(fullPath.data());
   m_fileIndex[fileKey] = translationUnitPos;

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

         translationUnit.forEachRecordWithSymbol(key, [&results](const Record* record) {
            if (record->attributes.isDefinition)
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

         translationUnit.forEachRecordWithSymbol(key, [&results](const Record* record) {
            if (!record->attributes.isDefinition)
            {
               results.push_back(record);
            }
         });
      }
   }

   return results;
}

std::vector<const ftags::Record*> ftags::ProjectDb::findReference(const std::string& symbolName) const
{
   std::vector<const ftags::Record*> results;

   const auto key = m_symbolTable.getKey(symbolName.data());
   if (key)
   {
      const auto range = m_symbolIndex.equal_range(key);
      for (auto translationUnitPos = range.first; translationUnitPos != range.second; ++translationUnitPos)
      {
         const auto& translationUnit = m_translationUnits.at(translationUnitPos->second);

         translationUnit.forEachRecordWithSymbol(key, [&results](const Record* record) {
            if (record->attributes.isReference)
            {
               results.push_back(record);
            }
         });
      }
   }

   return results;
}
