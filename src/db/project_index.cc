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

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

const ftags::ProjectDb::TranslationUnit& ftags::ProjectDb::parseOneFile(const std::string&       fileName,
                                                                        std::vector<const char*> arguments,
                                                                        bool                     includeEverything)
{
   try
   {
      std::string filterPath;
      if (!includeEverything)
      {
         filterPath = m_root;
      }

      TranslationUnit::ParsingContext parsingContext{
         m_symbolTable, m_namespaceTable, m_fileNameTable, m_recordSpanManager, filterPath};

      ftags::ProjectDb::TranslationUnit translationUnit =
         ftags::ProjectDb::TranslationUnit::parse(fileName, arguments, parsingContext);

      spdlog::debug("Loaded {:n} records from {}, {:n} from main file",
                    translationUnit.getRecordCount(m_recordSpanManager),
                    fileName,
                    translationUnit.getRecords(true, m_recordSpanManager).size());

      const TranslationUnitStore::size_type translationUnitPos = m_translationUnits.size();

      m_translationUnits.push_back(translationUnit);

      m_fileIndex[translationUnit.getFileNameKey()] = translationUnitPos;

      return m_translationUnits.back();
   }
   catch (const std::runtime_error& re)
   {
      spdlog::error("Failed to parse {}: {}", re.what());
      throw re;
   }
}
