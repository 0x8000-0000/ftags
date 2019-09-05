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
#include <statistics.h>

#include <fmt/format.h>

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

   if (m_translationUnits.size() != other.m_translationUnits.size())
   {
      return false;
   }

   std::map<std::string, std::vector<const Record*>> thisTranslationUnits;
   std::map<std::string, std::vector<const Record*>> otherTranslationUnits;

   for (const auto& translationUnit : m_translationUnits)
   {
      const std::string_view translationUnitName = m_fileNameTable.getStringView(translationUnit.getFileNameKey());

      const std::vector<const Record*> records = translationUnit.getRecords(false, m_recordSpanManager);

      thisTranslationUnits.emplace(std::string(translationUnitName), records);
   }

   for (const auto& translationUnit : other.m_translationUnits)
   {
      const std::string_view translationUnitName =
         other.m_fileNameTable.getStringView(translationUnit.getFileNameKey());

      const std::vector<const Record*> records = translationUnit.getRecords(false, other.m_recordSpanManager);

      otherTranslationUnits.emplace(std::string(translationUnitName), records);
   }

   if (thisTranslationUnits.size() != otherTranslationUnits.size())
   {
      return false;
   }

   auto thisIter  = thisTranslationUnits.cbegin();
   auto otherIter = otherTranslationUnits.cbegin();

   while ((thisIter != thisTranslationUnits.cend()) && (otherIter != otherTranslationUnits.cend()))
   {
      if (thisIter->first == otherIter->first) // filenames match
      {
         if (thisIter->second.size() == otherIter->second.size()) // record counts match
         {
            const CursorSet thisCursorSet(thisIter->second, m_symbolTable, m_fileNameTable);
            const CursorSet otherCursorSet(otherIter->second, other.m_symbolTable, other.m_fileNameTable);

            assert(thisCursorSet.size() == otherCursorSet.size());

            auto thisCursorIter  = thisCursorSet.begin();
            auto otherCursorIter = thisCursorSet.begin();

            while ((thisCursorIter != thisCursorSet.end()) && (otherCursorIter != otherCursorSet.end()))
            {
               Cursor thisCursor  = thisCursorSet.inflateRecord(*thisCursorIter);
               Cursor otherCursor = otherCursorSet.inflateRecord(*otherCursorIter);

               if (strcmp(thisCursor.symbolName, otherCursor.symbolName) != 0)
               {
                  return false;
               }

               if (strcmp(thisCursor.location.fileName, otherCursor.location.fileName) != 0)
               {
                  return false;
               }

               if (thisCursor.attributes.type != otherCursor.attributes.type)
               {
                  return false;
               }

               ++thisCursorIter;
               ++otherCursorIter;
            }
         }
      }
      else
      {
         return false;
      }

      ++thisIter;
      ++otherIter;
   }

   return true;
}

ftags::Cursor ftags::ProjectDb::inflateRecord(const ftags::Record* record) const
{
   ftags::Cursor cursor{};

   cursor.symbolName = m_symbolTable.getString(record->symbolNameKey);

   cursor.location.fileName = m_fileNameTable.getString(record->location.fileNameKey);
   cursor.location.line     = record->location.line;
   cursor.location.column   = record->location.column;

   cursor.attributes = record->attributes;

   return cursor;
}

#if 0
const ftags::ProjectDb::TranslationUnit&
ftags::ProjectDb::addTranslationUnit(const std::string&       fullPath,
                                     const TranslationUnit&   translationUnit,
                                     const RecordSpanManager& otherRecordSpanManager)
{
   /*
    * add the new translation unit to database
    */
   const TranslationUnitStore::size_type translationUnitPos = m_translationUnits.size();

   const StringTable::Key translationUnitFileKey = m_fileNameTable.addKey(fullPath.data());

   /*
    * clone the records using the project's symbol table
    */
   m_translationUnits.emplace_back(translationUnitFileKey);

   m_translationUnits.back().copyRecords(translationUnit, otherRecordSpanManager, m_recordSpanManager);

   /*
    * register the name of the translation unit
    */
   const auto fileKey   = m_fileNameTable.addKey(fullPath.data());
   m_fileIndex[fileKey] = translationUnitPos;

   assert(translationUnit.getRecordCount(otherRecordSpanManager) == m_translationUnits.back().getRecordCount(m_recordSpanManager));

   assert(translationUnit.getRecords(true, otherRecordSpanManager).size() == m_translationUnits.back().getRecords(true, m_recordSpanManager).size());

   assert(m_translationUnits.back().isValid());

   return m_translationUnits.back();
}
#endif

bool ftags::ProjectDb::isFileIndexed(const std::string& fileName) const
{
   bool       isIndexed = false;
   const auto key       = m_fileNameTable.getKey(fileName.data());
   if (key != 0)
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
      translationUnit.forEachRecord(
         [&functions](const Record* record) {
            if (record->attributes.getType() == SymbolType::FunctionDeclaration)
            {
               functions.push_back(record);
            }
         },
         m_recordSpanManager);
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
      symbolName, [symbolType](const Record* record) { return record->attributes.getType() == symbolType; });
}

std::vector<const ftags::Record*>
ftags::ProjectDb::identifySymbol(const std::string& fileName, unsigned lineNumber, unsigned columnNumber) const
{
   const auto key = m_fileNameTable.getKey(fileName.data());

   return m_recordSpanManager.findClosestRecord(key, m_symbolTable, lineNumber, columnNumber);
}

std::vector<std::vector<const ftags::Record*>> ftags::ProjectDb::identifySymbolExtended(const std::string& fileName,
                                                                                        unsigned           lineNumber,
                                                                                        unsigned columnNumber) const
{
   std::vector<const ftags::Record*> symbolRecords = identifySymbol(fileName, lineNumber, columnNumber);

   std::vector<std::vector<const ftags::Record*>> results;

   std::for_each(symbolRecords.cbegin(), symbolRecords.cend(), [&results, this](const Record* record) {
      std::vector<const ftags::Record*> otherRefs = m_recordSpanManager.findClosestRecord(
         record->definition.fileNameKey, m_symbolTable, record->definition.line, record->definition.column);

      otherRefs.push_back(record);

      results.emplace_back(std::move(otherRefs));
   });

   return results;
}

std::vector<const ftags::Record*> ftags::ProjectDb::dumpTranslationUnit(const std::string& fileName) const
{
   const ftags::util::StringTable::Key      fileKey            = m_fileNameTable.getKey(fileName.data());
   const auto                               translationUnitPos = m_fileIndex.at(fileKey);
   const ftags::ProjectDb::TranslationUnit& translationUnit    = m_translationUnits.at(translationUnitPos);

   return translationUnit.getRecords(true, m_recordSpanManager);
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
         0U,
         [](std::size_t acc, const TranslationUnit& elem) { return acc + elem.computeSerializedSize(); });

   return sizeof(ftags::util::SerializedObjectHeader) +
          ftags::util::Serializer<std::string>::computeSerializedSize(m_name) +
          ftags::util::Serializer<std::string>::computeSerializedSize(m_root) +
          m_fileNameTable.computeSerializedSize() + m_symbolTable.computeSerializedSize() +
          m_namespaceTable.computeSerializedSize() + m_recordSpanManager.computeSerializedSize() + sizeof(uint64_t) +
          translationUnitSize;
}

void ftags::ProjectDb::serialize(ftags::util::BufferInsertor& insertor) const
{
   ftags::util::SerializedObjectHeader header{"ftags::ProjectDb"};
   insertor << header;

   ftags::util::Serializer<std::string>::serialize(m_name, insertor);
   ftags::util::Serializer<std::string>::serialize(m_root, insertor);

   m_fileNameTable.serialize(insertor);
   m_symbolTable.serialize(insertor);
   m_namespaceTable.serialize(insertor);
   m_recordSpanManager.serialize(insertor);

   const uint64_t translationUnitCount = m_translationUnits.size();
   insertor << translationUnitCount;

   std::for_each(m_translationUnits.cbegin(),
                 m_translationUnits.cend(),
                 [&insertor](const TranslationUnit& translationUnit) { translationUnit.serialize(insertor); });
}

ftags::ProjectDb ftags::ProjectDb::deserialize(ftags::util::BufferExtractor& extractor)
{
   ftags::util::SerializedObjectHeader header;
   extractor >> header;

   const std::string name = ftags::util::Serializer<std::string>::deserialize(extractor);
   const std::string root = ftags::util::Serializer<std::string>::deserialize(extractor);

   ftags::ProjectDb projectDb(name, root);

   projectDb.m_fileNameTable     = ftags::util::StringTable::deserialize(extractor);
   projectDb.m_symbolTable       = ftags::util::StringTable::deserialize(extractor);
   projectDb.m_namespaceTable    = ftags::util::StringTable::deserialize(extractor);
   projectDb.m_recordSpanManager = RecordSpanManager::deserialize(extractor);

   uint64_t translationUnitCount = 0;
   extractor >> translationUnitCount;
   projectDb.m_translationUnits.reserve(translationUnitCount);

   for (size_t ii = 0; ii < translationUnitCount; ii++)
   {
      projectDb.m_translationUnits.push_back(TranslationUnit::deserialize(extractor));
      projectDb.m_fileIndex[projectDb.m_translationUnits.back().getFileNameKey()] = ii;
   }

   projectDb.assertValid();

   return projectDb;
}

void ftags::ProjectDb::mergeFrom(const ProjectDb& other)
{
   /*
    * merge the symbols
    */
   KeyMap symbolKeyMapping   = m_symbolTable.mergeStringTable(other.m_symbolTable);
   KeyMap fileNameKeyMapping = m_fileNameTable.mergeStringTable(other.m_fileNameTable);

   for (const auto& otherTranslationUnit : other.m_translationUnits)
   {
      const TranslationUnitStore::size_type translationUnitPos = m_translationUnits.size();

      assert(otherTranslationUnit.getFileNameKey());
      const auto iter = fileNameKeyMapping.lookup(otherTranslationUnit.getFileNameKey());
      assert(iter != fileNameKeyMapping.none());

      m_translationUnits.emplace_back(iter->first);
      m_translationUnits.back().copyRecords(
         otherTranslationUnit, other.m_recordSpanManager, m_recordSpanManager, symbolKeyMapping, fileNameKeyMapping);

      /*
       * register the name of the translation unit
       */
      m_fileIndex[iter->first] = translationUnitPos;
   }
}

void ftags::ProjectDb::updateFrom(const std::string& /* fileName */, const ProjectDb& other)
{
   // TODO(signbit): check if the file name is indexed already and remove its entries
   mergeFrom(other);
}

constexpr uint32_t k_ExtraLargeSymbolSize      = 1024;
constexpr int      k_NumberOfHugeSymbolsToDump = 16;
constexpr uint32_t k_SizeOfHugeSymbolPrefix    = 128;

std::vector<std::string> ftags::ProjectDb::getStatisticsRemarks(const std::string& statisticsGroup) const
{
   std::vector<std::string> remarks;

   if (statisticsGroup == "recordspans")
   {
      remarks = m_recordSpanManager.getStatisticsRemarks();
   }
   else if (statisticsGroup == "symbols")
   {
      ftags::stats::Sample<unsigned> symbolSizes;

      m_symbolTable.forEachElement([&symbolSizes](std::string_view symbol, ftags::util::StringTable::Key /* key */) {
         symbolSizes.addValue(static_cast<unsigned>(symbol.size()));
      });

      const ftags::stats::FiveNumbersSummary<unsigned> symbolSizesSummary = symbolSizes.computeFiveNumberSummary();

      remarks.emplace_back(fmt::format("Indexed {:n} symbols", m_symbolTable.getSize()));
      remarks.emplace_back("Symbol sizes, (five number summary):");
      remarks.emplace_back(fmt::format("  minimum:        {:>8}", symbolSizesSummary.minimum));
      remarks.emplace_back(fmt::format("  lower quartile: {:>8}", symbolSizesSummary.lowerQuartile));
      remarks.emplace_back(fmt::format("  median:         {:>8}", symbolSizesSummary.median));
      remarks.emplace_back(fmt::format("  upper quartile: {:>8}", symbolSizesSummary.upperQuartile));
      remarks.emplace_back(fmt::format("  maximum:        {:>8}", symbolSizesSummary.maximum));
      remarks.emplace_back("");
   }
   else if (statisticsGroup == "debug_symbols")
   {
      std::vector<ftags::util::StringTable::Key> largeSymbols;

      m_symbolTable.forEachElement([&largeSymbols](std::string_view symbol, ftags::util::StringTable::Key key) {
         if (symbol.size() > k_ExtraLargeSymbolSize)
         {
            largeSymbols.push_back(key);
         }
      });

      remarks.emplace_back(fmt::format("Found {:n} symbols larger than 1024", largeSymbols.size()));

      std::sort(largeSymbols.begin(), largeSymbols.end());

      std::vector<const Record*> recordsWithLargeSymbols;
      m_recordSpanManager.forEachRecord([&largeSymbols, &recordsWithLargeSymbols](const Record* record) {
         if (std::binary_search(largeSymbols.cbegin(), largeSymbols.cend(), record->symbolNameKey))
         {
            recordsWithLargeSymbols.push_back(record);
         }
      });

      remarks.emplace_back(
         fmt::format("Found {:n} records with symbols larger than 1024", recordsWithLargeSymbols.size()));

      int top16 = k_NumberOfHugeSymbolsToDump;
      for (const auto* record : recordsWithLargeSymbols)
      {
         if (top16 > 0)
         {
            top16--;

            remarks.emplace_back(fmt::format("  ... {}:{}:{}",
                                             m_fileNameTable.getStringView(record->location.fileNameKey),
                                             record->location.line,
                                             record->location.column));

            remarks.emplace_back(fmt::format(
               "  \\ {}", m_symbolTable.getStringView(record->symbolNameKey).substr(0, k_SizeOfHugeSymbolPrefix)));
         }
         else
         {
            break;
         }
      }
   }
   else
   {
      remarks.emplace_back(fmt::format("Serialized size is {:n} bytes", computeSerializedSize()));
      remarks.emplace_back(fmt::format("Indexed {:n} translation units", m_translationUnits.size()));
      remarks.emplace_back(fmt::format("Indexed {:n} symbols", m_symbolTable.getSize()));
      remarks.emplace_back(fmt::format("Indexed {:n} distinct files", m_fileNameTable.getSize()));
   }

   return remarks;
}

#if (!defined(NDEBUG)) && (defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
void ftags::ProjectDb::assertValid() const
{
   m_recordSpanManager.assertValid();

   assert(m_fileNameTable.getSize() >= m_translationUnits.size());

   assert(m_recordSpanManager.getSymbolCount() == m_symbolTable.getSize());

   for (const auto& translationUnit : m_translationUnits)
   {
      translationUnit.assertValid();
   }

   for (const auto& iter : m_fileIndex)
   {
      assert(iter.first == m_translationUnits[iter.second].getFileNameKey());
   }

   struct SymbolCount
   {
      const StringTable& symbolTable;
      const StringTable& fileNameTable;

      uint64_t invalidSymbols;
      uint64_t invalidFileNames;

      uint64_t missingSymbols;
      uint64_t missingFiles;
   } counts{m_symbolTable, m_fileNameTable, 0u, 0u, 0u, 0u};

   m_recordSpanManager.forEachRecord([&counts](const Record* record) {
      if (record->symbolNameKey == 0)
      {
         counts.invalidSymbols++;
      }
      else
      {
         if (counts.symbolTable.getString(record->symbolNameKey) == nullptr)
         {
            counts.missingSymbols++;
         }
      }

      if (record->location.fileNameKey == 0)
      {
         counts.invalidFileNames++;
      }
      else
      {
         if (counts.fileNameTable.getString(record->location.fileNameKey) == nullptr)
         {
            counts.missingFiles++;
         }
      }
   });

   assert(counts.invalidFileNames == 0);

   assert(counts.missingFiles == 0);

   assert(counts.invalidSymbols == 0);

   assert(counts.missingSymbols == 0);

   /*
    * it's all good
    */
}
#endif

std::vector<std::string> ftags::ProjectDb::analyzeData(const std::string& analysisType) const
{
   std::vector<std::string> remarks;

   if (analysisType == "recordspans")
   {
      remarks = m_recordSpanManager.analyzeRecordSpans(m_symbolTable, m_fileNameTable);
   }
   else if (analysisType == "records")
   {
      remarks = m_recordSpanManager.analyzeRecords();
   }
   else
   {
      remarks.emplace_back(fmt::format("Analysis of '{}' complete.", analysisType));
   }

   return remarks;
}
