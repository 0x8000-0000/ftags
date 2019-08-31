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

void ftags::ProjectDb::TranslationUnit::beginParsingUnit(StringTable::Key fileNameKey)
{
   m_fileNameKey = fileNameKey;
}

void ftags::ProjectDb::TranslationUnit::finalizeParsingUnit(RecordSpanManager& recordSpanCache)
{
   flushCurrentSpan(recordSpanCache);
}

void ftags::ProjectDb::TranslationUnit::copyRecords(const TranslationUnit&   otherTranslationUnit,
                                                    const RecordSpanManager& otherRecordSpanManager,
                                                    RecordSpanManager&       recordSpanManager)
{
   /*
    * copy the original records
    */
   m_recordSpans.reserve(otherTranslationUnit.m_recordSpans.size());

   std::vector<Record> tempRecords;

   for (RecordSpan::Store::Key otherKey : otherTranslationUnit.m_recordSpans)
   {
      const RecordSpan& otherSpan = otherRecordSpanManager.getSpan(otherKey);
      otherSpan.copyRecordsTo(tempRecords);

      m_recordSpans.push_back(recordSpanManager.addSpan(tempRecords));
   }
}

void ftags::ProjectDb::TranslationUnit::copyRecords(const TranslationUnit&   otherTranslationUnit,
                                                    const RecordSpanManager& otherRecordSpanManager,
                                                    RecordSpanManager&       recordSpanManager,
                                                    const KeyMap&            symbolKeyMapping,
                                                    const KeyMap&            fileNameKeyMapping)
{
   /*
    * copy the original records
    */
   m_recordSpans.reserve(otherTranslationUnit.m_recordSpans.size());

   for (RecordSpan::Store::Key otherKey : otherTranslationUnit.m_recordSpans)
   {
      std::vector<Record> tempRecords;

      const RecordSpan& otherSpan = otherRecordSpanManager.getSpan(otherKey);
      otherSpan.copyRecordsTo(tempRecords);

      RecordSpan::filterRecords(tempRecords, symbolKeyMapping, fileNameKeyMapping);

      m_recordSpans.push_back(recordSpanManager.addSpan(tempRecords));
   }
}

void ftags::ProjectDb::TranslationUnit::flushCurrentSpan(RecordSpanManager& recordSpanManager)
{
   if (!m_currentSpan.empty())
   {
      m_recordSpans.push_back(recordSpanManager.addSpan(m_currentSpan));

      m_currentSpan.clear();
      m_currentSpanLocations.clear();
   }
}

void ftags::ProjectDb::TranslationUnit::addCursor(const ftags::Cursor&      cursor,
                                                  ftags::StringTable::Key   symbolNameKey,
                                                  ftags::StringTable::Key   fileNameKey,
                                                  ftags::StringTable::Key   referencedFileNameKey,
                                                  ftags::RecordSpanManager& recordSpanManager)
{
   if (cursor.attributes.type == ftags::SymbolType::DeclarationReferenceExpression)
   {
      assert(!m_currentSpan.empty());

      ftags::Record& oldRecord = m_currentSpan.back();

      if (oldRecord.attributes.type == ftags::SymbolType::FunctionCallExpression)
      {
         if (oldRecord.symbolNameKey == symbolNameKey)
         {
            // skip over this record since it is a duplicate FunctionCallExpression / DeclarationReferenceExpression
            return;
         }
      }
   }

   if (cursor.attributes.type == ftags::SymbolType::NamespaceReference)
   {
      assert(!m_currentSpan.empty());

      ftags::Record& oldRecord = m_currentSpan.back();

      oldRecord.namespaceKey = symbolNameKey;

      return;
   }

   if (fileNameKey != m_currentRecordSpanFileKey)
   {
      /*
       * Open a new span because the file name key is different
       */
      flushCurrentSpan(recordSpanManager);

      m_currentRecordSpanFileKey = fileNameKey;
   }

   const SymbolAtLocation newLocation{symbolNameKey, fileNameKey, cursor.location.line, cursor.location.column};

   if (m_currentSpanLocations.find(newLocation) == m_currentSpanLocations.end())
   {
      m_currentSpanLocations.insert(newLocation);

      ftags::Record& newRecord = m_currentSpan.emplace_back();

      newRecord.symbolNameKey = symbolNameKey;
      newRecord.attributes    = cursor.attributes;

      newRecord.setLocationFileKey(fileNameKey);
      newRecord.setLocationAddress(cursor.location.line, cursor.location.column);

      newRecord.setDefinitionFileKey(referencedFileNameKey);
      newRecord.setDefinitionAddress(cursor.definition.line, cursor.definition.column);
   }
}

std::size_t ftags::ProjectDb::TranslationUnit::computeSerializedSize() const
{
   std::vector<uint64_t> recordSpanHashes(/* size = */ m_recordSpans.size());

   return sizeof(ftags::SerializedObjectHeader) + sizeof(StringTable::Key) +
          ftags::Serializer<std::vector<RecordSpan::Store::Key>>::computeSerializedSize(m_recordSpans);
}

void ftags::ProjectDb::TranslationUnit::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header{"ftags::TranslationUnit"};
   insertor << header;

   assert(m_fileNameKey);
   insertor << m_fileNameKey;

   std::vector<uint64_t> recordSpanHashes;

   ftags::Serializer<std::vector<RecordSpan::Store::Key>>::serialize(m_recordSpans, insertor);
}

ftags::ProjectDb::TranslationUnit ftags::ProjectDb::TranslationUnit::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::ProjectDb::TranslationUnit retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   extractor >> retval.m_fileNameKey;
   assert(retval.m_fileNameKey);

   retval.m_recordSpans = ftags::Serializer<std::vector<RecordSpan::Store::Key>>::deserialize(extractor);

   return retval;
}

#if (!defined(NDEBUG)) && (defined(ENABLE_THOROUGH_VALIDITY_CHECKS))
void ftags::ProjectDb::TranslationUnit::assertValid() const
{
   assert(m_fileNameKey != 0);
}
#endif
