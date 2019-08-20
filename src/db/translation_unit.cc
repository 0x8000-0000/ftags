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

void ftags::ProjectDb::TranslationUnit::updateIndices()
{
   std::for_each(
      m_recordSpans.begin(), m_recordSpans.end(), [](std::shared_ptr<RecordSpan>& rs) { rs->updateIndices(); });
}

void ftags::ProjectDb::TranslationUnit::copyRecords(const TranslationUnit& original, RecordSpanCache& spanCache)
{
   /*
    * copy the original records
    */
   m_recordSpans.reserve(original.m_recordSpans.size());

   for (const std::shared_ptr<RecordSpan>& other : original.m_recordSpans)
   {
      std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>();
      newSpan->addRecords(*other);
      std::shared_ptr<RecordSpan> sharedSpan = spanCache.add(newSpan);
      m_recordSpans.push_back(sharedSpan);
   }
}

void ftags::ProjectDb::TranslationUnit::copyRecords(const TranslationUnit& original,
                                                    RecordSpanCache&       spanCache,
                                                    const KeyMap&          symbolKeyMapping,
                                                    const KeyMap&          fileNameKeyMapping)
{
   /*
    * copy the original records
    */
   m_recordSpans.reserve(original.m_recordSpans.size());

   for (const std::shared_ptr<RecordSpan>& other : original.m_recordSpans)
   {
      std::shared_ptr<RecordSpan> newSpan = std::make_shared<RecordSpan>();
      newSpan->addRecords(*other, symbolKeyMapping, fileNameKeyMapping);
      std::shared_ptr<RecordSpan> sharedSpan = spanCache.add(newSpan);
      m_recordSpans.push_back(sharedSpan);
   }
}

void ftags::ProjectDb::TranslationUnit::addCursor(const ftags::Cursor&    cursor,
                                                  ftags::StringTable::Key symbolNameKey,
                                                  ftags::StringTable::Key fileNameKey,
                                                  ftags::StringTable::Key referencedFileNameKey)
{
   ftags::Record newRecord = {};

   newRecord.symbolNameKey = symbolNameKey;
   newRecord.attributes    = cursor.attributes;

   newRecord.setLocationFileKey(fileNameKey);
   newRecord.setLocationAddress(cursor.location.line, cursor.location.column);

   if (newRecord.location.fileNameKey != m_currentRecordSpanFileKey)
   {
      /*
       * Open a new span because the file name key is different
       */
      m_recordSpans.push_back(std::make_shared<RecordSpan>());
      m_currentRecordSpanFileKey = newRecord.location.fileNameKey;
   }

   newRecord.setDefinitionFileKey(referencedFileNameKey);
   newRecord.setDefinitionAddress(cursor.definition.line, cursor.definition.column);

   m_recordSpans.back()->addRecord(newRecord);
}

std::size_t ftags::ProjectDb::TranslationUnit::computeSerializedSize() const
{
   std::vector<uint64_t> recordSpanHashes(/* size = */ m_recordSpans.size());

   return sizeof(ftags::SerializedObjectHeader) +
          ftags::Serializer<std::vector<uint64_t>>::computeSerializedSize(recordSpanHashes);
}

void ftags::ProjectDb::TranslationUnit::serialize(ftags::BufferInsertor& insertor) const
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

ftags::ProjectDb::TranslationUnit ftags::ProjectDb::TranslationUnit::deserialize(ftags::BufferExtractor& extractor,
                                                                                 const RecordSpanCache&  spanCache)
{
   ftags::ProjectDb::TranslationUnit retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   std::vector<uint64_t> recordSpanHashes = ftags::Serializer<std::vector<uint64_t>>::deserialize(extractor);

   retval.m_recordSpans.reserve(recordSpanHashes.size());

   for (const uint64_t spanHash : recordSpanHashes)
   {
      std::vector<std::shared_ptr<RecordSpan>> spans = spanCache.get(spanHash);
      if (spans.size() == 1)
      {
         retval.m_recordSpans.push_back(spans[0]);
      }
      else
      {
         assert(false);
      }
   }

   return retval;
}
