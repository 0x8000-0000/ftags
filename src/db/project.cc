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

#include <SpookyV2.h>

#include <algorithm>
#include <numeric>

namespace
{

class OrderRecordsBySymbolKey
{
public:
   OrderRecordsBySymbolKey(const std::vector<ftags::Record>& records) : m_records{records}
   {
   }

   bool operator()(const unsigned& left, const unsigned& right) const
   {
      const ftags::Record& leftRecord  = m_records[left];
      const ftags::Record& rightRecord = m_records[right];

      if (leftRecord.symbolNameKey < rightRecord.symbolNameKey)
      {
         return true;
      }

      if (leftRecord.symbolNameKey == rightRecord.symbolNameKey)
      {
         if (leftRecord.attributes.type < rightRecord.attributes.type)
         {
            return true;
         }
      }

      return false;
   }

private:
   const std::vector<ftags::Record>& m_records;
};

bool compareRecordsByLocation(const ftags::Record& leftRecord, const ftags::Record& rightRecord)
{
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

class OrderRecordsByFileKey
{
public:
   OrderRecordsByFileKey(const std::vector<ftags::Record>& records) : m_records{records}
   {
   }

   bool operator()(const unsigned& left, const unsigned& right) const
   {
      const ftags::Record& leftRecord  = m_records[left];
      const ftags::Record& rightRecord = m_records[right];

      return compareRecordsByLocation(leftRecord, rightRecord);
   }

private:
   const std::vector<ftags::Record>& m_records;
};

#if 0
void filterDuplicates(std::vector<const ftags::Record*> records)
{
   const auto begin = records.begin();
   const auto end   = records.end();

   std::sort(begin, end, [](const ftags::Record* leftRecord, const ftags::Record* rightRecord) {
      return compareRecordsByLocation(*leftRecord, *rightRecord);
   });

   auto last = std::unique(begin, end);
   records.erase(last, end);
}
#endif

} // anonymous namespace

void ftags::RecordSpan::updateIndices()
{
   m_recordsInSymbolKeyOrder.resize(m_records.size());
   std::iota(m_recordsInSymbolKeyOrder.begin(), m_recordsInSymbolKeyOrder.end(), 0);
   std::sort(m_recordsInSymbolKeyOrder.begin(), m_recordsInSymbolKeyOrder.end(), OrderRecordsBySymbolKey(m_records));
}

void ftags::TranslationUnit::updateIndices()
{
   std::for_each(
      m_recordSpans.begin(), m_recordSpans.end(), [](std::shared_ptr<RecordSpan>& rs) { rs->updateIndices(); });
}

void ftags::RecordSpan::addRecords(const RecordSpan&               other,
                                   const ftags::FlatMap<Key, Key>& symbolKeyMapping,
                                   const ftags::FlatMap<Key, Key>& fileNameKeyMapping)
{
   m_records = other.m_records;

   for (auto& record : m_records)
   {
      {
         auto fileNameIter = fileNameKeyMapping.lookup(record.fileNameKey);
         assert(fileNameIter != fileNameKeyMapping.none());
         record.fileNameKey = fileNameIter->second;
      }

      {
         auto symbolNameIter = symbolKeyMapping.lookup(record.symbolNameKey);
         assert(symbolNameIter != symbolKeyMapping.none());
         record.symbolNameKey = symbolNameIter->second;
      }
   }

   updateIndices();

   m_hash = SpookyHash::Hash64(m_records.data(), m_records.size() * sizeof(Record), k_hashSeed);
}

std::shared_ptr<ftags::RecordSpan> ftags::RecordSpanCache::add(std::shared_ptr<ftags::RecordSpan> original)
{
   m_spansObserved++;

   auto range = m_cache.equal_range(original->getHash());

   for (auto iter = range.first; iter != range.second; ++iter)
   {
      auto elem = iter->second;

      std::shared_ptr<RecordSpan> val = elem.lock();

      if (!val)
      {
         continue;
      }
      else
      {
         if (val->operator==(*original))
         {
            return val;
         }
      }
   }

   m_cache.emplace(original->getHash(), original);

   /*
    * gather all unique symbols in this record span
    */
   std::set<ftags::StringTable::Key> symbolKeys;
   original->forEachRecord(
      [&symbolKeys](const Record* record) { symbolKeys.insert(record->symbolNameKey); });

   /*
    * add a mapping from this symbol to this record span
    */
   std::for_each(
      symbolKeys.cbegin(), symbolKeys.cend(), [this, original](ftags::StringTable::Key symbolKey) {
         m_symbolIndex.emplace(symbolKey, original);
      });

   return original;
}

void ftags::TranslationUnit::copyRecords(const TranslationUnit& original, RecordSpanCache& spanCache)
{
   /*
    * replace keys from translation unit's table with keys from the project
    * database
    */

   ftags::FlatMap<Key, Key> symbolKeyMapping   = m_symbolTable.mergeStringTable(original.m_symbolTable);
   ftags::FlatMap<Key, Key> fileNameKeyMapping = m_fileNameTable.mergeStringTable(original.m_fileNameTable);

   /*
    * copy the original records
    */
   m_recordSpans.reserve(original.m_recordSpans.size());

#if 0
   std::for_each(
      original.m_recordSpans.cbegin(),
      original.m_recordSpans.cend(),
      [symbolKeyMapping, fileNameKeyMapping, spanCache, this](const std::shared_ptr<RecordSpan>& recordSpan) {
         std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>(m_symbolTable, m_fileNameTable);
         newSpan->addRecords(*recordSpan, symbolKeyMapping, fileNameKeyMapping);
         std::shared_ptr<RecordSpan> sharedSpan = spanCache.add(newSpan);
         m_recordSpans.push_back(sharedSpan);
      });
#else
   for (const std::shared_ptr<RecordSpan>& other : original.m_recordSpans)
   {
      std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>(m_symbolTable, m_fileNameTable);
      newSpan->addRecords(*other, symbolKeyMapping, fileNameKeyMapping);
      std::shared_ptr<RecordSpan> sharedSpan = spanCache.add(newSpan);
      m_recordSpans.push_back(sharedSpan);
   }
#endif
}

void ftags::TranslationUnit::addCursor(const ftags::Cursor& cursor, const ftags::Attributes& attributes)
{
   ftags::Record newRecord = {};

   newRecord.symbolNameKey = m_symbolTable.addKey(cursor.symbolName);
   newRecord.fileNameKey   = m_fileNameTable.addKey(cursor.location.fileName);

   newRecord.attributes      = attributes;
   newRecord.attributes.type = cursor.symbolType;

   newRecord.startLine   = static_cast<uint32_t>(cursor.location.line);
   newRecord.startColumn = static_cast<uint16_t>(cursor.location.column);
   newRecord.endLine     = static_cast<uint16_t>(cursor.endLine);

   if (newRecord.fileNameKey != m_currentRecordSpanFileKey)
   {
      /*
       * Open a new span because the file name key is different
       */
      m_recordSpans.push_back(std::make_shared<RecordSpan>(m_symbolTable, m_fileNameTable));
      m_currentRecordSpanFileKey = newRecord.fileNameKey;
   }

   m_recordSpans.back()->addRecord(newRecord);
}

/*
 * ProjectDb
 */

ftags::ProjectDb::ProjectDb() : m_operatingState{OptimizedForParse}
{
}

ftags::Cursor ftags::ProjectDb::inflateRecord(const ftags::Record* record) const
{
   ftags::Cursor cursor{};

   cursor.symbolName = m_symbolTable.getString(record->symbolNameKey);

   cursor.location.fileName = m_fileNameTable.getString(record->fileNameKey);
   cursor.location.line     = static_cast<int>(record->startLine);
   cursor.location.column   = record->startColumn;

   return cursor;
}

void ftags::ProjectDb::addTranslationUnit(const std::string& fullPath, const TranslationUnit& translationUnit)
{
   /*
    * protect access
    */
   std::lock_guard<std::mutex> lock(m_updateTranslationUnits);

   /*
    * clone the records using the project's symbol table
    */
   TranslationUnit newTranslationUnit{m_symbolTable, m_fileNameTable};
   newTranslationUnit.copyRecords(translationUnit, m_recordSpanCache);

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

ftags::CursorSet ftags::ProjectDb::inflateRecords(const std::vector<const Record*>& records) const
{
   ftags::CursorSet retval(records, m_symbolTable, m_fileNameTable);

   return retval;
}

/*
 * CursorSet
 */
ftags::CursorSet::CursorSet(std::vector<const Record*> records,
                            const StringTable&         symbolTable,
                            const StringTable&         fileNameTable)
{
   m_records.reserve(records.size());

   for (auto record : records)
   {
      m_records.push_back(*record);
      auto& newRecord = m_records.back();

      const char* symbolName = symbolTable.getString(record->symbolNameKey);
      const char* fileName   = fileNameTable.getString(record->fileNameKey);

      newRecord.symbolNameKey = m_symbolTable.addKey(symbolName);
      newRecord.fileNameKey   = m_fileNameTable.addKey(fileName);
   }
}

ftags::Cursor ftags::CursorSet::inflateRecord(const ftags::Record& record) const
{
   ftags::Cursor cursor{};

   cursor.symbolName = m_symbolTable.getString(record.symbolNameKey);

   cursor.location.fileName = m_fileNameTable.getString(record.fileNameKey);
   cursor.location.line     = static_cast<int>(record.startLine);
   cursor.location.column   = record.startColumn;

   return cursor;
}

/*
 * Record serialization
 */
template <>
std::size_t
ftags::Serializer<std::vector<ftags::Record>>::computeSerializedSize(const std::vector<ftags::Record>& val)
{
   return sizeof(ftags::SerializedObjectHeader) + sizeof(uint64_t) + val.size() * sizeof(ftags::Record);
}

template <>
void ftags::Serializer<std::vector<ftags::Record>>::serialize(const std::vector<ftags::Record>& val,
                                                              ftags::BufferInsertor&            insertor)
{
   ftags::SerializedObjectHeader header{"std::vector<ftags::Record>"};
   insertor << header;

   const uint64_t vecSize = val.size();
   insertor << vecSize;

   insertor << val;
}

template <>
std::vector<ftags::Record>
ftags::Serializer<std::vector<ftags::Record>>::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::SerializedObjectHeader header;
   extractor >> header;

   uint64_t vecSize = 0;
   extractor >> vecSize;

   std::vector<ftags::Record> retval(/* size = */ vecSize);
   extractor >> retval;

   return retval;
}
