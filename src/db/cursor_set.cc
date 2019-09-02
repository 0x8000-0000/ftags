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

#include <spooky.h>

/*
 * CursorSet
 */
ftags::CursorSet::CursorSet(std::vector<const Record*>      records,
                            const ftags::util::StringTable& symbolTable,
                            const ftags::util::StringTable& fileNameTable)
{
   m_records.reserve(records.size());

   for (auto record : records)
   {
      m_records.push_back(*record);
      auto& newRecord = m_records.back();

      {
         const std::string_view symbolName = symbolTable.getStringView(record->symbolNameKey);
         newRecord.symbolNameKey           = m_symbolTable.addKey(symbolName);
      }

      {
         const std::string_view fileName = fileNameTable.getStringView(record->location.fileNameKey);
         newRecord.setLocationFileKey(m_fileNameTable.addKey(fileName));
      }

      {
         const std::string_view referenceFileName = fileNameTable.getStringView(record->definition.fileNameKey);
         newRecord.setDefinitionFileKey(m_fileNameTable.addKey(referenceFileName));
      }
   }
}

ftags::Cursor ftags::CursorSet::inflateRecord(const ftags::Record& record) const
{
   ftags::Cursor cursor{};

   cursor.symbolName = m_symbolTable.getString(record.symbolNameKey);

   cursor.location.fileName = m_fileNameTable.getString(record.location.fileNameKey);
   cursor.location.line     = record.location.line;
   cursor.location.column   = record.location.column;

   cursor.definition.fileName = m_fileNameTable.getString(record.definition.fileNameKey);
   cursor.definition.line     = record.definition.line;
   cursor.definition.column   = record.definition.column;

   cursor.attributes = record.attributes;

   return cursor;
}

std::size_t ftags::CursorSet::computeSerializedSize() const
{
   return sizeof(ftags::util::SerializedObjectHeader) +
          ftags::util::Serializer<std::vector<Record>>::computeSerializedSize(m_records) +
          m_symbolTable.computeSerializedSize() + m_fileNameTable.computeSerializedSize();
}

void ftags::CursorSet::serialize(ftags::util::BufferInsertor& insertor) const
{
   ftags::util::SerializedObjectHeader header{"ftags::CursorSet"};
   insertor << header;

   ftags::util::Serializer<std::vector<Record>>::serialize(m_records, insertor);

   m_symbolTable.serialize(insertor);

   m_fileNameTable.serialize(insertor);
}

ftags::CursorSet ftags::CursorSet::deserialize(ftags::util::BufferExtractor& extractor)
{
   ftags::CursorSet retval;

   ftags::util::SerializedObjectHeader header;
   extractor >> header;

   retval.m_records = ftags::util::Serializer<std::vector<Record>>::deserialize(extractor);

   retval.m_symbolTable = ftags::util::StringTable::deserialize(extractor);

   retval.m_fileNameTable = ftags::util::StringTable::deserialize(extractor);

   return retval;
}

std::size_t ftags::CursorSet::computeHash() const
{
   SpookyHash hash;

   hash.Init(k_hashSeed[0], k_hashSeed[1]);

   for (auto& record : m_records)
   {
      Cursor cursor = inflateRecord(record);

      hash.Update(cursor.symbolName, strlen(cursor.symbolName));
      hash.Update(cursor.location.fileName, strlen(cursor.location.fileName));

      cursor.symbolName        = nullptr;
      cursor.location.fileName = nullptr;

      hash.Update(&cursor, sizeof(cursor));
   }

   uint64_t hashValue[2] = {};

   hash.Final(&hashValue[0], &hashValue[1]);

   return hashValue[0];
}
