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

ftags::Record* ftags::ProjectDb::addCursor(const ftags::Cursor& cursor, const ftags::Attributes& /* attributes */)
{
   ftags::Record newRecord;
   newRecord.fileNameKey   = m_fileNameTable.addKey(cursor.location.fileName);
   newRecord.symbolNameKey = m_symbolTable.addKey(cursor.symbolName);

   auto iter = m_records.insert(newRecord);

   m_fileIndex[newRecord.fileNameKey].push_back(iter);
   m_symbolIndex[newRecord.symbolNameKey].push_back(iter);

   return &*iter;
}

bool ftags::ProjectDb::isFileIndexed(const std::string& fileName) const
{
   bool isIndexed = false;
   uint32_t key = m_fileNameTable.getKey(fileName.data());
   if (key)
   {
      isIndexed = m_fileIndex.count(key) != 0;
   }
   return isIndexed;
}

std::vector<ftags::Record*> ftags::ProjectDb::getFunctions() const
{
   std::vector<ftags::Record*> functions;
   return functions;
}
