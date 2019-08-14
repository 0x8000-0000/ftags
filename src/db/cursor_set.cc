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

   cursor.attributes = record.attributes;

   return cursor;
}

std::size_t ftags::CursorSet::computeSerializedSize() const
{
   return sizeof(ftags::SerializedObjectHeader) +
          ftags::Serializer<std::vector<Record>>::computeSerializedSize(m_records) +
          m_symbolTable.computeSerializedSize() + m_fileNameTable.computeSerializedSize();
}

void ftags::CursorSet::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::CursorSet"};
   insertor << header;

   ftags::Serializer<std::vector<Record>>::serialize(m_records, insertor);

   m_symbolTable.serialize(insertor);

   m_fileNameTable.serialize(insertor);
}

ftags::CursorSet ftags::CursorSet::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::CursorSet retval;

   ftags::SerializedObjectHeader header;
   extractor >> header;

   retval.m_records = ftags::Serializer<std::vector<Record>>::deserialize(extractor);

   retval.m_symbolTable = StringTable::deserialize(extractor);

   retval.m_fileNameTable = StringTable::deserialize(extractor);

   return retval;
}
