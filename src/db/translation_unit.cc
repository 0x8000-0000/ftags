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

void ftags::TranslationUnit::updateIndices()
{
   std::for_each(
      m_recordSpans.begin(), m_recordSpans.end(), [](std::shared_ptr<RecordSpan>& rs) { rs->updateIndices(); });
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

void ftags::TranslationUnit::addCursor(const ftags::Cursor& cursor)
{
   ftags::Record newRecord = {};

   newRecord.symbolNameKey = m_symbolTable.addKey(cursor.symbolName);
   newRecord.fileNameKey   = m_fileNameTable.addKey(cursor.location.fileName);

   newRecord.attributes = cursor.attributes;

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
