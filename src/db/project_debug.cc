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
#include <ostream>

namespace
{

class OrderRecordsByLocation
{
public:
   OrderRecordsByLocation(const std::vector<ftags::Record>& records) : m_records{records}
   {
   }

   bool operator()(const unsigned& left, const unsigned& right) const
   {
      assert(right < m_records.size());
      assert(left < m_records.size());

      const ftags::Record& leftRecord  = m_records[left];
      const ftags::Record& rightRecord = m_records[right];

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

private:
   const std::vector<ftags::Record>& m_records;
};


} // anonymous namespace

void ftags::RecordSpan::dumpRecords(std::ostream& os) const
{
   std::for_each(m_records.cbegin(), m_records.cend(), [&os, this](const Record& record) {
      const char* symbolName = getSymbolName(record);
      const char* fileName   = getFileName(record);
      std::string symbolType = record.attributes.getRecordType();
      os << "    " << symbolName << "    " << symbolType << "   " << fileName << ':' << record.startLine << ':'
         << record.startColumn << std::endl;
   });
}

void ftags::TranslationUnit::dumpRecords(std::ostream& os) const
{
   os << " Found " << getRecordCount() << " records." << std::endl;

   std::for_each(m_recordSpans.cbegin(), m_recordSpans.cend(), [&os](const std::shared_ptr<RecordSpan>& rs) { rs->dumpRecords(os); });
}

void ftags::ProjectDb::dumpRecords(std::ostream& os) const
{
   std::for_each(
      m_translationUnits.cbegin(), m_translationUnits.cend(), [&os, this](const TranslationUnit& translationUnit) {
         const auto  fileNameKey = translationUnit.getFileNameKey();
         const char* fileName    = m_fileNameTable.getString(fileNameKey);
         if (fileName)
         {
            os << "File: " << fileName << std::endl;
         }
         else
         {
            os << "File: unnamed" << std::endl;
         }
         translationUnit.dumpRecords(os);
      });
}

void ftags::ProjectDb::dumpStats(std::ostream& os) const
{
   os << "ProjectDb stats: ";
   os << "Cache utilization: " << m_recordSpanCache.getActiveSpanCount() << " live spans out of " << m_recordSpanCache.getTotalSpanCount();
   os << std::endl;
}
