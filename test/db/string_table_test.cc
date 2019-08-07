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

TEST(StringTableTest, EmptyTableHasNoString)
{
   ftags::StringTable st;

   const uint32_t keyForInvalidValue = st.getKey("foo");

   ASSERT_EQ(keyForInvalidValue, 0);
}

TEST(StringTableTest, AddOneGetItBack)
{
   ftags::StringTable st;

   const char fooString[] = "foo";

   st.addKey(fooString);

   const uint32_t keyForFoo = st.getKey(fooString);

   ASSERT_NE(keyForFoo, 0);

   const char* key = st.getString(keyForFoo);
   ASSERT_STREQ(key, fooString);
}

TEST(StringTableTest, DataStructureOwnsIt)
{
   ftags::StringTable st;

   {
      std::string fooString{"foo"};

      st.addKey(fooString.data());
   }

   const uint32_t keyForFoo = st.getKey("foo");

   ASSERT_NE(keyForFoo, 0);
}

TEST(StringTableTest, AddTwiceGetSomeIndexBack)
{
   ftags::StringTable st;

   const char fooString[] = "foo";

   const uint32_t firstPosition = st.addKey(fooString);

   const uint32_t keyForFoo = st.getKey(fooString);

   ASSERT_EQ(keyForFoo, firstPosition);

   const uint32_t secondPosition = st.addKey(fooString);

   ASSERT_EQ(secondPosition, firstPosition);
}

TEST(StringTableTest, AddTwoAndGetThemBack)
{
   ftags::StringTable st;

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

TEST(StringTableTest, Serialize)
{
   ftags::StringTable st;

   const char fooString[] = "foo";
   const char barString[] = "bar";

   st.addKey(fooString);
   st.addKey(barString);

   const auto estimatedSerializedSize = st.computeSerializedSize();

   std::vector<std::byte> serializedFormat(/* size = */ estimatedSerializedSize);

   ftags::BufferInsertor insertor{serializedFormat};
   st.serialize(insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor extractor{serializedFormat};
   ftags::StringTable rec = ftags::StringTable::deserialize(extractor);
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

