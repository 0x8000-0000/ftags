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

#include <gtest/gtest.h>

using ftags::util::BufferExtractor;
using ftags::util::BufferInsertor;
using ftags::util::FlatMap;
using ftags::util::StringTable;

TEST(StringTableTest, EmptyTableHasNoString)
{
   StringTable st;

   const uint32_t keyForInvalidValue = st.getKey("foo");

   ASSERT_EQ(keyForInvalidValue, 0);
}

TEST(StringTableTest, AddOneGetItBack)
{
   StringTable st;

   const char fooString[] = "foo";

   st.addKey(fooString);

   const uint32_t keyForFoo = st.getKey(fooString);

   ASSERT_NE(keyForFoo, 0);

   const char* key = st.getString(keyForFoo);
   ASSERT_STREQ(key, fooString);
}

TEST(StringTableTest, DataStructureOwnsIt)
{
   StringTable st;

   {
      std::string fooString{"foo"};

      st.addKey(fooString.data());
   }

   const uint32_t keyForFoo = st.getKey("foo");

   ASSERT_NE(keyForFoo, 0);
}

TEST(StringTableTest, AddTwiceGetSomeIndexBack)
{
   StringTable st;

   const char fooString[] = "foo";

   const uint32_t firstPosition = st.addKey(fooString);

   const uint32_t keyForFoo = st.getKey(fooString);

   ASSERT_EQ(keyForFoo, firstPosition);

   const uint32_t secondPosition = st.addKey(fooString);

   ASSERT_EQ(secondPosition, firstPosition);
}

TEST(StringTableTest, AddTwoAndGetThemBack)
{
   StringTable st;

   const char fooString[] = "foo";
   const char barString[] = "bar";

   st.addKey(fooString);
   st.addKey(barString);

   const uint32_t keyForFoo = st.getKey(fooString);
   ASSERT_NE(keyForFoo, 0);
   const char* fooKey = st.getString(keyForFoo);
   ASSERT_STREQ(fooKey, fooString);

   const uint32_t keyForBar = st.getKey(barString);
   ASSERT_NE(keyForBar, 0);
   const char* barKey = st.getString(keyForBar);
   ASSERT_STREQ(barKey, barString);
}

TEST(StringTableTest, SerializeTwoStrings)
{
   StringTable st;

   const char fooString[] = "foo";
   const char barString[] = "bar";

   st.addKey(fooString);
   st.addKey(barString);

   const auto estimatedSerializedSize = st.computeSerializedSize();

   std::vector<std::byte> serializedFormat(/* size = */ estimatedSerializedSize);

   BufferInsertor insertor{serializedFormat};
   st.serialize(insertor);
   insertor.assertEmpty();

   ftags::util::BufferExtractor extractor{serializedFormat};
   StringTable                  rec = StringTable::deserialize(extractor);
   extractor.assertEmpty();

   const uint32_t keyForFoo = st.getKey(fooString);

   const uint32_t rec_keyForFoo = rec.getKey(fooString);
   ASSERT_EQ(keyForFoo, rec_keyForFoo);
   const char* rec_fooKey = rec.getString(rec_keyForFoo);
   ASSERT_STREQ(rec_fooKey, fooString);

   const uint32_t keyForBar = st.getKey(barString);

   const uint32_t rec_keyForBar = rec.getKey(barString);
   ASSERT_EQ(keyForBar, rec_keyForBar);
   const char* rec_barKey = rec.getString(rec_keyForBar);
   ASSERT_STREQ(rec_barKey, barString);
}

TEST(StringTableTest, SerializeTwoStringsWithGap)
{
   StringTable st;

   const char fooString[]   = "foo";
   const char alphaString[] = "alpha";
   const char barString[]   = "bar";

   st.addKey(fooString);
   st.addKey(alphaString);
   st.addKey(barString);

   st.removeKey(alphaString);

   const auto estimatedSerializedSize = st.computeSerializedSize();

   std::vector<std::byte> serializedFormat(/* size = */ estimatedSerializedSize);

   BufferInsertor insertor{serializedFormat};
   st.serialize(insertor);
   insertor.assertEmpty();

   BufferExtractor extractor{serializedFormat};
   StringTable     rec = StringTable::deserialize(extractor);
   extractor.assertEmpty();

   const uint32_t keyForFoo = st.getKey(fooString);

   const uint32_t rec_keyForFoo = rec.getKey(fooString);
   ASSERT_EQ(keyForFoo, rec_keyForFoo);
   const char* rec_fooKey = rec.getString(rec_keyForFoo);
   ASSERT_STREQ(rec_fooKey, fooString);

   const uint32_t keyForBar = st.getKey(barString);

   const uint32_t rec_keyForBar = rec.getKey(barString);
   ASSERT_EQ(keyForBar, rec_keyForBar);
   const char* rec_barKey = rec.getString(rec_keyForBar);
   ASSERT_STREQ(rec_barKey, barString);
}

TEST(StringTableTest, MergeStringTables)
{
   using Key = StringTable::Key;

   StringTable left;
   StringTable right;

   const char fooString[] = "foo";
   const char barString[] = "bar";
   const char bazString[] = "baz";

   left.addKey(fooString);
   left.addKey(barString);

   right.addKey(barString);
   right.addKey(bazString);

   ASSERT_EQ(left.getKey(bazString), 0);
   const uint32_t oldKeyForFoo = left.getKey(fooString);
   const uint32_t oldKeyForBar = left.getKey(barString);

   FlatMap<Key, Key> mapping = left.mergeStringTable(right);

   ASSERT_NE(left.getKey(bazString), 0);
   ASSERT_EQ(oldKeyForFoo, left.getKey(fooString));
   ASSERT_EQ(oldKeyForBar, left.getKey(barString));

   Key  barKeyInRight      = right.getKey(barString);
   auto barKeyInMergedIter = mapping.lookup(barKeyInRight);
   ASSERT_NE(mapping.none(), barKeyInMergedIter);
   Key barKeyInMerged = barKeyInMergedIter->second;
   ASSERT_STREQ(barString, left.getString(barKeyInMerged));

   Key  bazKeyInRight      = right.getKey(bazString);
   auto bazKeyInMergedIter = mapping.lookup(bazKeyInRight);
   ASSERT_NE(mapping.none(), bazKeyInMergedIter);
   Key bazKeyInMerged = bazKeyInMergedIter->second;
   ASSERT_STREQ(bazString, left.getString(bazKeyInMerged));
}

TEST(StringTableTest, AddTenThousandStrings)
{
   using Key = StringTable::Key;

   StringTable stringTable;

   std::string input = "abcdefghijlkmnopqrstuvxyz";

   const int testSize = 10 * 1000;

   for (int ii = 0; ii < testSize; ii++)
   {
      stringTable.addKey(input.data());
      std::next_permutation(input.begin(), input.end());
   }

   const size_t           serializedSize = stringTable.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ serializedSize);

   BufferInsertor insertor{buffer};
   stringTable.serialize(insertor);
   insertor.assertEmpty();

   BufferExtractor extractor{buffer};
   StringTable     rehydrated = StringTable::deserialize(extractor);

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
