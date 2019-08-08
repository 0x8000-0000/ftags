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

std::size_t ftags::CursorSet::computeSerializedSize() const
{
   return sizeof(ftags::SerializedObjectHeader) +
          ftags::Serializer<std::vector<Record>>::computeSerializedSize(m_records) +
          m_symbolTable.computeSerializedSize() + m_fileNameTable.computeSerializedSize();
}

void ftags::CursorSet::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header = {};
   insertor << header;

   ftags::Serializer<std::vector<Record>>::serialize(m_records, insertor);

   insertor << m_symbolTable;

   insertor << m_fileNameTable;
}

ftags::CursorSet ftags::CursorSet::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::CursorSet retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   retval.m_records = ftags::Serializer<std::vector<Record>>::deserialize(extractor);

   retval.m_symbolTable = StringTable::deserialize(extractor);

   retval.m_fileNameTable = StringTable::deserialize(extractor);

   return retval;
}
