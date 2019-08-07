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

#include <string_table.h>

#include <serialization.h>

std::size_t ftags::StringTable::computeSerializedSize() const
{
   return sizeof(ftags::SerializedObjectHeader) + m_store.computeSerializedSize();
}

void ftags::StringTable::serialize(ftags::BufferInsertor& insertor) const
{
   ftags::SerializedObjectHeader header = {};
   insertor << header;

   m_store.serialize(insertor);
}

ftags::StringTable ftags::StringTable::deserialize(ftags::BufferExtractor& extractor)
{
   ftags::StringTable retval;

   ftags::SerializedObjectHeader header = {};
   extractor >> header;

   retval.m_store = StoreType::deserialize(extractor);

   /*
    * traverse m_store and find the strings
    */
   auto allocSeq = retval.m_store.getFirstAllocatedSequence();
   while (retval.m_store.isValidAllocatedSequence(allocSeq))
   {
      auto iter = retval.m_store.get(allocSeq.key).first;

      const char* symbol = &*iter;
      StoreType::block_size_type key = allocSeq.key;
      for (StoreType::block_size_type ii = 0; ii < allocSeq.size; ii++)
      {
         if (*iter == '\0')
         {
            retval.m_index[symbol] = key;

            ++ iter;

            symbol = &*iter;
            key = allocSeq.key + ii + 1;
         }
         else
         {
            ++ iter;
         }
      }

      retval.m_store.getNextAllocatedSequence(allocSeq);
   }

   return retval;
}
