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

#include <ostream>

void ftags::TranslationUnit::dumpRecords(std::ostream& os) const
{
   os << " Found " << m_records.size() << " records." << std::endl;
   for (const auto& record : m_records)
   {
      const char* symbolName = getSymbolName(record);
      const char* fileName   = getFileName(record);
      os << "    " << symbolName << "    " << fileName << ':' << record.startLine << ':' << record.startColumn
         << std::endl;
   }
   os << " ----- " << std::endl;
}

void ftags::ProjectDb::dumpRecords(std::ostream& os) const
{
   for (const auto& translationUnit : m_translationUnits)
   {
      const auto fileNameKey = translationUnit.getFileNameKey();
      const char* fileName   = m_fileNameTable.getString(fileNameKey);
      if (fileName)
      {
         os << "File: " << fileName << std::endl;
      }
      else
      {
         os << "File: unnamed" << std::endl;
      }
      translationUnit.dumpRecords(os);
   }
}
