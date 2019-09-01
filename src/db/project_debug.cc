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

#include <fmt/format.h>

#include <algorithm>
#include <filesystem>
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

      if (leftRecord.location.fileNameKey < rightRecord.location.fileNameKey)
      {
         return true;
      }

      if (leftRecord.location.fileNameKey == rightRecord.location.fileNameKey)
      {
         if (leftRecord.location.line < rightRecord.location.line)
         {
            return true;
         }

         if (leftRecord.location.line == rightRecord.location.line)
         {
            if (leftRecord.location.column < rightRecord.location.column)
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

void ftags::RecordSpan::dumpRecords(std::ostream&                   os,
                                    const ftags::util::StringTable& symbolTable,
                                    const ftags::util::StringTable& fileNameTable,
                                    const std::filesystem::path&    trimPath) const
{
   for (std::size_t ii = 0; ii < m_size; ii++)
   {
      const Record& record = m_records[ii];

      std::string namespaceName;
      if (record.namespaceKey)
      {
         namespaceName = fmt::format("{}::", symbolTable.getString(record.namespaceKey));
      }
      const char* symbolName = symbolTable.getString(record.symbolNameKey);
      // const char*       fileName   = fileNameTable.getString(record.location.fileNameKey);
      const std::string           symbolType = record.attributes.getRecordType();
      const std::filesystem::path filePath{fileNameTable.getString(record.location.fileNameKey)};
      os << fmt::format("   {}{}  {} {} {}:{}",
                        namespaceName,
                        symbolName,
                        symbolType,
                        std::filesystem::relative(filePath, trimPath).string(),
                        record.location.line,
                        record.location.column)
         << std::endl;
   }
}

void ftags::ProjectDb::TranslationUnit::dumpRecords(std::ostream&                   os,
                                                    const RecordSpanManager&        recordSpanManager,
                                                    const ftags::util::StringTable& symbolTable,
                                                    const ftags::util::StringTable& fileNameTable,
                                                    const std::filesystem::path&    trimPath) const
{
   os << " Found " << getRecordCount(recordSpanManager) << " records." << std::endl;

   forEachRecordSpan(
      [&os, &symbolTable, &fileNameTable, &trimPath](const RecordSpan& recordSpan) {
         recordSpan.dumpRecords(os, symbolTable, fileNameTable, trimPath);
      },
      recordSpanManager);
}

void ftags::ProjectDb::dumpRecords(std::ostream& os, const std::filesystem::path& trimPath) const
{
   std::for_each(m_translationUnits.cbegin(),
                 m_translationUnits.cend(),
                 [&os, &trimPath, this](const TranslationUnit& translationUnit) {
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
                    translationUnit.dumpRecords(os, m_recordSpanManager, m_symbolTable, m_fileNameTable, trimPath);
                 });
}

void ftags::ProjectDb::dumpStats(std::ostream& os) const
{
   os << "ProjectDb stats: ";
#if 0
   os << "Cache utilization: " << m_recordSpanManager.getActiveSpanCount() << " live spans out of "
      << m_recordSpanManager.getRecordCount();
#endif
   os << std::endl;
}
