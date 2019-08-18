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

/*
 * ProjectDb
 */

bool ftags::ProjectDb::operator==(const ftags::ProjectDb& other) const
{
   if (this == &other)
   {
      return true;
   }

   if ((m_symbolTable == other.m_symbolTable) && (m_fileNameTable == other.m_fileNameTable))
   {
      if (m_translationUnits.size() == other.m_translationUnits.size())
      {
         std::multiset<std::size_t> myTranslationHashes;
         std::multiset<std::size_t> otherTranslationHashes;

         for (const auto& translationUnit : m_translationUnits)
         {
            const std::vector<const Record*> records = translationUnit.getRecords(false);

            const CursorSet cursor(records, m_symbolTable, m_fileNameTable);

            const std::size_t cursorHash = cursor.computeHash();

            myTranslationHashes.insert(cursorHash);
         }

         for (const auto& translationUnit : other.m_translationUnits)
         {
            const std::vector<const Record*> records = translationUnit.getRecords(false);

            const CursorSet cursor(records, other.m_symbolTable, other.m_fileNameTable);

            const std::size_t cursorHash = cursor.computeHash();

            otherTranslationHashes.insert(cursorHash);
         }

         if (myTranslationHashes.size() == otherTranslationHashes.size())
         {
            auto myIter    = myTranslationHashes.cbegin();
            auto otherIter = otherTranslationHashes.cbegin();

            while ((myIter != myTranslationHashes.cend()) && (otherIter != otherTranslationHashes.cend()))
            {
               if (*myIter == *otherIter)
               {
                  ++myIter;
                  ++otherIter;
               }
               else
               {
                  return false;
               }
            }
         }
      }
   }

   return true;
}

ftags::Cursor ftags::ProjectDb::inflateRecord(const ftags::Record* record) const
{
   ftags::Cursor cursor{};

   cursor.symbolName = m_symbolTable.getString(record->symbolNameKey);

   cursor.location.fileName = m_fileNameTable.getString(record->fileNameKey);
   cursor.location.line     = static_cast<int>(record->startLine);
   cursor.location.column   = record->startColumn;

   cursor.attributes = record->attributes;

   return cursor;
}

const ftags::TranslationUnit& ftags::ProjectDb::addTranslationUnit(const std::string&     fullPath,
                                                                   const TranslationUnit& translationUnit,
                                                                   const StringTable&     symbolTable,
                                                                   const StringTable&     fileNameTable)
{
   /*
    * protect access
    */
   std::lock_guard<std::mutex> lock(m_updateTranslationUnits);

   /*
    * add the new translation unit to database
    */
   const TranslationUnitStore::size_type translationUnitPos = m_translationUnits.size();

   /*
    * clone the records using the project's symbol table
    */
   m_translationUnits.emplace_back(/* m_symbolTable, m_fileNameTable */);

   /*
    * merge the other symbols into this database
    */
   KeyMap symbolKeyMapping   = m_symbolTable.mergeStringTable(symbolTable);
   KeyMap fileNameKeyMapping = m_fileNameTable.mergeStringTable(fileNameTable);

   m_translationUnits.back().copyRecords(translationUnit, m_recordSpanCache, symbolKeyMapping, fileNameKeyMapping);

   /*
    * register the name of the translation unit
    */
   const auto fileKey   = m_fileNameTable.addKey(fullPath.data());
   m_fileIndex[fileKey] = translationUnitPos;

   assert(translationUnit.getRecordCount() == m_translationUnits.back().getRecordCount());

   assert(translationUnit.getRecords(true).size() == m_translationUnits.back().getRecords(true).size());

   return m_translationUnits.back();
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
         if (record->attributes.type == SymbolType::FunctionDeclaration)
         {
            functions.push_back(record);
         }
      });
   }

   return functions;
}

std::vector<const ftags::Record*> ftags::ProjectDb::findDefinition(const std::string& symbolName) const
{
   return filterRecordsWithSymbol(symbolName, [](const Record* record) { return record->attributes.isDefinition; });
}

std::vector<const ftags::Record*> ftags::ProjectDb::findDeclaration(const std::string& symbolName) const
{
   return filterRecordsWithSymbol(symbolName, [](const Record* record) {
      return record->attributes.isDeclaration && (!record->attributes.isDefinition);
   });
}

std::vector<const ftags::Record*> ftags::ProjectDb::findReference(const std::string& symbolName) const
{
   return filterRecordsWithSymbol(symbolName, [](const Record* record) { return record->attributes.isReference; });
}

std::vector<const ftags::Record*> ftags::ProjectDb::findSymbol(const std::string& symbolName,
                                                               ftags::SymbolType  symbolType) const
{
   return filterRecordsWithSymbol(
      symbolName, [symbolType](const Record* record) { return record->attributes.type == symbolType; });
}

std::vector<const ftags::Record*> ftags::ProjectDb::dumpTranslationUnit(const std::string& fileName) const
{
   const StringTable::Key        fileKey            = m_fileNameTable.getKey(fileName.data());
   const auto                    translationUnitPos = m_fileIndex.at(fileKey);
   const ftags::TranslationUnit& translationUnit    = m_translationUnits.at(translationUnitPos);

   return translationUnit.getRecords(true);
}

ftags::CursorSet ftags::ProjectDb::inflateRecords(const std::vector<const Record*>& records) const
{
   ftags::CursorSet retval(records, m_symbolTable, m_fileNameTable);

   return retval;
}

std::size_t ftags::ProjectDb::computeSerializedSize() const
{
   const std::size_t translationUnitSize =

      std::accumulate(
         m_translationUnits.cbegin(),
         m_translationUnits.cend(),
         0u,
         [](std::size_t acc, const TranslationUnit& elem) { return acc + elem.computeSerializedSize(); });

   return sizeof(ftags::SerializedObjectHeader) + m_fileNameTable.computeSerializedSize() +
          m_symbolTable.computeSerializedSize() + m_namespaceTable.computeSerializedSize() +
          m_recordSpanCache.computeSerializedSize() + sizeof(uint64_t) + translationUnitSize;
}

void ftags::ProjectDb::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::ProjectDb"};
   insertor << header;

   m_fileNameTable.serialize(insertor);
   m_symbolTable.serialize(insertor);
   m_namespaceTable.serialize(insertor);

   m_recordSpanCache.serialize(insertor);

   const uint64_t translationUnitCount = m_translationUnits.size();
   insertor << translationUnitCount;

   std::for_each(m_translationUnits.cbegin(),
                 m_translationUnits.cend(),
                 [&insertor](const TranslationUnit& translationUnit) { translationUnit.serialize(insertor); });
}

void ftags::ProjectDb::deserialize(ftags::BufferExtractor& extractor, ftags::ProjectDb& projectDb)
{
   ftags::SerializedObjectHeader header;
   extractor >> header;

   projectDb.m_fileNameTable  = StringTable::deserialize(extractor);
   projectDb.m_symbolTable    = StringTable::deserialize(extractor);
   projectDb.m_namespaceTable = StringTable::deserialize(extractor);

   std::vector<std::shared_ptr<RecordSpan>> hardReferences;

   projectDb.m_recordSpanCache = RecordSpanCache::deserialize(extractor, hardReferences);

   uint64_t translationUnitCount = 0;
   extractor >> translationUnitCount;
   projectDb.m_translationUnits.reserve(translationUnitCount);

   for (size_t ii = 0; ii < translationUnitCount; ii++)
   {
      projectDb.m_translationUnits.push_back(TranslationUnit::deserialize(extractor, projectDb.m_recordSpanCache));
   }
}

void ftags::ProjectDb::mergeFrom(const ProjectDb& /* other */)
{
}

