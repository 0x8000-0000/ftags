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

#include <serialization_legacy.h>
#include <string_table.h>

#include <gtest/gtest.h>

using ftags::util::StringTable;
using Key = ftags::util::StringTable::Key;
using ftags::util::BufferExtractor;
using ftags::util::BufferInsertor;

TEST(StringTableTest, AddOneMillionStrings)
{
   StringTable stringTable;

   std::string input = "abcdefghijlkmnopqrstuvxyz";

   const int testSize = 1000 * 1000;

   for (int ii = 0; ii < testSize; ii++)
   {
      stringTable.addKey(input.data());
      std::next_permutation(input.begin(), input.end());
   }

   const size_t           serializedSize = stringTable.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ serializedSize);

   BufferInsertor insertor{buffer};
   stringTable.serialize(insertor.getInsertor());
   insertor.assertEmpty();

   BufferExtractor extractor{buffer};
   StringTable     rehydrated = StringTable::deserialize(extractor.getExtractor());

   std::string test = "abcdefghijlkmnopqrstuvxyz";

   for (int ii = 0; ii < testSize; ii++)
   {
      Key originalKey   = stringTable.getKey(test.data());
      Key rehydratedKey = rehydrated.getKey(test.data());

      ASSERT_NE(originalKey, 0);

      ASSERT_EQ(rehydratedKey, originalKey);

      std::next_permutation(test.begin(), test.end());
   }
}

TEST(StringTableTest, AddTenMillionStrings)
{
   StringTable stringTable;

   std::string input = "abcdefghijlkmnopqrstuvxyz";

   const int testSize = 10 * 1000 * 1000;

   for (int ii = 0; ii < testSize; ii++)
   {
      stringTable.addKey(input.data());
      std::next_permutation(input.begin(), input.end());
   }

   const size_t           serializedSize = stringTable.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ serializedSize);

   BufferInsertor insertor{buffer};
   stringTable.serialize(insertor.getInsertor());
   insertor.assertEmpty();

   BufferExtractor extractor{buffer};
   StringTable     rehydrated = StringTable::deserialize(extractor.getExtractor());

   std::string test = "abcdefghijlkmnopqrstuvxyz";

   for (int ii = 0; ii < testSize; ii++)
   {
      Key originalKey   = stringTable.getKey(test.data());
      Key rehydratedKey = rehydrated.getKey(test.data());

      ASSERT_NE(originalKey, 0);

      ASSERT_EQ(rehydratedKey, originalKey);

      std::next_permutation(test.begin(), test.end());
   }
}
