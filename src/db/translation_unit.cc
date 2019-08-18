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

void ftags::TranslationUnit::copyRecords(const TranslationUnit& original,
                                         RecordSpanCache&       spanCache,
                                         const KeyMap&          symbolKeyMapping,
                                         const KeyMap&          fileNameKeyMapping)
{
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
      std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>();
      newSpan->addRecords(*other, symbolKeyMapping, fileNameKeyMapping);
      std::shared_ptr<RecordSpan> sharedSpan = spanCache.add(newSpan);
      m_recordSpans.push_back(sharedSpan);
   }
#endif
}

void ftags::TranslationUnit::addCursor(const ftags::Cursor&    cursor,
                                       ftags::StringTable::Key symbolNameKey,
                                       ftags::StringTable::Key fileNameKey)
{
   ftags::Record newRecord = {};

   newRecord.symbolNameKey = symbolNameKey;
   newRecord.fileNameKey   = fileNameKey;

   newRecord.attributes = cursor.attributes;

   newRecord.startLine   = static_cast<uint32_t>(cursor.location.line);
   newRecord.startColumn = static_cast<uint16_t>(cursor.location.column);
   newRecord.endLine     = static_cast<uint16_t>(cursor.endLine);

   if (newRecord.fileNameKey != m_currentRecordSpanFileKey)
   {
      /*
       * Open a new span because the file name key is different
       */
      m_recordSpans.push_back(std::make_shared<RecordSpan>());
      m_currentRecordSpanFileKey = newRecord.fileNameKey;
   }

   m_recordSpans.back()->addRecord(newRecord);
}

std::size_t ftags::TranslationUnit::computeSerializedSize() const
{
   std::vector<uint64_t> recordSpanHashes(/* size = */ m_recordSpans.size());

   return sizeof(ftags::SerializedObjectHeader) +
          ftags::Serializer<std::vector<uint64_t>>::computeSerializedSize(recordSpanHashes);
}

void ftags::TranslationUnit::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::TranslationUnit"};
   insertor << header;

   std::vector<uint64_t> recordSpanHashes;
   recordSpanHashes.reserve(m_recordSpans.size());

   std::for_each(
      m_recordSpans.cbegin(), m_recordSpans.cend(), [&recordSpanHashes](const std::shared_ptr<RecordSpan>& elem) {
         recordSpanHashes.push_back(elem->getHash());
      });

   ftags::Serializer<std::vector<uint64_t>>::serialize(recordSpanHashes, insertor);
}

void ftags::TranslationUnit::deserialize(ftags::BufferExtractor& extractor,
                                         ftags::TranslationUnit& translationUnit,
                                         const RecordSpanCache&  spanCache)
{
   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   std::vector<uint64_t> recordSpanHashes = ftags::Serializer<std::vector<uint64_t>>::deserialize(extractor);

   translationUnit.m_recordSpans.reserve(recordSpanHashes.size());

   for (const uint64_t spanHash : recordSpanHashes)
   {
      std::vector<std::shared_ptr<RecordSpan>> spans = spanCache.get(spanHash);
      if (spans.size() == 1)
      {
         translationUnit.m_recordSpans.push_back(spans[0]);
      }
      else
      {
         assert(false);
      }
   }
}
