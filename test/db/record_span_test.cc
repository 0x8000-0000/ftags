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

#include <record_span_manager.h>

#include <gtest/gtest.h>

TEST(RecordSpanManagerTest, RecordVector)
{
   std::vector<ftags::Record> input;

   ftags::Record one   = {};
   ftags::Record two   = {};
   ftags::Record three = {};

   one.symbolNameKey   = 1;
   two.symbolNameKey   = 2;
   three.symbolNameKey = 3;

   input.push_back(one);
   input.push_back(two);
   input.push_back(three);

   using Serializer = ftags::Serializer<std::vector<ftags::Record>>;

   const size_t           inputSerializedSize = Serializer::computeSerializedSize(input);
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};

   Serializer::serialize(input, insertor);
   insertor.assertEmpty();

   ftags::BufferExtractor           extractor{buffer};
   const std::vector<ftags::Record> output = Serializer::deserialize(extractor);
   extractor.assertEmpty();

   ASSERT_EQ(1, output[0].symbolNameKey);
   ASSERT_EQ(2, output[1].symbolNameKey);
   ASSERT_EQ(3, output[2].symbolNameKey);
}

TEST(RecordSpanManagerTest, ManageVector)
{
   ftags::RecordSpanManager manager;

   std::vector<ftags::Record> input;

   ftags::Record one   = {};
   ftags::Record two   = {};
   ftags::Record three = {};

   one.symbolNameKey   = 1;
   two.symbolNameKey   = 2;
   three.symbolNameKey = 3;

   input.push_back(one);
   input.push_back(two);
   input.push_back(three);

   const ftags::RecordSpan::Store::Key key = manager.addSpan(input);
   ASSERT_NE(0, key);
}

TEST(RecordSpanManagerTest, HandleDuplicates)
{
   ftags::RecordSpanManager manager;

   ftags::RecordSpan::Store::Key key0 = 0;

   {
      std::vector<ftags::Record> input;

      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};

      one.symbolNameKey   = 1;
      two.symbolNameKey   = 2;
      three.symbolNameKey = 3;

      input.push_back(one);
      input.push_back(two);
      input.push_back(three);

      key0 = manager.addSpan(input);
   }

   ftags::RecordSpan::Store::Key key1 = 0;

   {
      std::vector<ftags::Record> input;

      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};

      one.symbolNameKey   = 1;
      two.symbolNameKey   = 2;
      three.symbolNameKey = 3;

      input.push_back(one);
      input.push_back(two);
      input.push_back(three);

      key1 = manager.addSpan(input);
   }

   ASSERT_EQ(key0, key1);
}

TEST(RecordSpanManagerTest, HandleDuplicatesAfterSerialization)
{
   ftags::RecordSpanManager manager;

   ftags::RecordSpan::Store::Key key0 = 0;

   {
      std::vector<ftags::Record> input;

      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};

      one.symbolNameKey   = 1;
      two.symbolNameKey   = 2;
      three.symbolNameKey = 3;

      input.push_back(one);
      input.push_back(two);
      input.push_back(three);

      key0 = manager.addSpan(input);
   }

   ftags::RecordSpanManager newManager;
   {
      const size_t           inputSerializedSize = manager.computeSerializedSize();
      std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

      ftags::BufferInsertor insertor{buffer};
      manager.serialize(insertor);

      ftags::BufferExtractor extractor{buffer};
      newManager = ftags::RecordSpanManager::deserialize(extractor);
   }

   ftags::RecordSpan::Store::Key key1 = 0;

   {
      std::vector<ftags::Record> input;

      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};

      one.symbolNameKey   = 1;
      two.symbolNameKey   = 2;
      three.symbolNameKey = 3;

      input.push_back(one);
      input.push_back(two);
      input.push_back(three);

      key1 = newManager.addSpan(input);
   }

   ASSERT_EQ(key0, key1);
}

TEST(RecordSpanManagerTest, RecordIteration)
{
   ftags::RecordSpanManager   manager;
   std::vector<ftags::Record> input;

   {
      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};
      ftags::Record four  = {};
      ftags::Record five  = {};

      one.symbolNameKey        = 1;
      two.symbolNameKey        = 2;
      two.location.fileNameKey = 45;
      three.symbolNameKey      = 3;
      four.symbolNameKey       = 3;
      five.symbolNameKey       = 2;
      two.location.fileNameKey = 25;

      input.push_back(one);
      input.push_back(two);
      input.push_back(three);
      input.push_back(four);
      input.push_back(five);
   }

   const ftags::RecordSpan::Store::Key key0 = manager.addSpan(input);

   std::vector<ftags::Record> input2;

   {
      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};

      one.symbolNameKey        = 1;
      two.symbolNameKey        = 2;
      two.location.fileNameKey = 99;
      three.symbolNameKey      = 3;

      input2.push_back(one);
      input2.push_back(two);
      input2.push_back(three);
   }

   const ftags::RecordSpan::Store::Key key1 = manager.addSpan(input2);

   ASSERT_NE(key0, key1);

   std::vector<const ftags::Record*> collector;

   manager.forEachRecordWithSymbol(2, [&collector](const ftags::Record* record) { collector.push_back(record); });
   ASSERT_EQ(3, collector.size());

   std::vector<const ftags::Record*> filtered =
      manager.filterRecordsWithSymbol(2, [](const ftags::Record* /* record */) { return true; });
   ASSERT_EQ(3, filtered.size());
}

TEST(RecordSpanManagerTest, RecordIterationAfterSerialization)
{
   ftags::RecordSpanManager   manager;
   std::vector<ftags::Record> input;

   {
      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};
      ftags::Record four  = {};
      ftags::Record five  = {};

      one.symbolNameKey         = 1;
      two.symbolNameKey         = 2;
      two.location.fileNameKey  = 45;
      three.symbolNameKey       = 3;
      four.symbolNameKey        = 3;
      five.symbolNameKey        = 2;
      five.location.fileNameKey = 25;

      input.push_back(one);
      input.push_back(two);
      input.push_back(three);
      input.push_back(four);
      input.push_back(five);
   }

   const ftags::RecordSpan::Store::Key key0 = manager.addSpan(input);

   const size_t           inputSerializedSize = manager.computeSerializedSize();
   std::vector<std::byte> buffer(/* size = */ inputSerializedSize);

   ftags::BufferInsertor insertor{buffer};
   manager.serialize(insertor);

   ftags::BufferExtractor extractor{buffer};

   ftags::RecordSpanManager newManager = ftags::RecordSpanManager::deserialize(extractor);

   std::vector<ftags::Record> input2;

   {
      ftags::Record one   = {};
      ftags::Record two   = {};
      ftags::Record three = {};

      one.symbolNameKey        = 1;
      two.symbolNameKey        = 2;
      two.location.fileNameKey = 99;
      three.symbolNameKey      = 3;

      input2.push_back(one);
      input2.push_back(two);
      input2.push_back(three);
   }

   const ftags::RecordSpan::Store::Key key1 = newManager.addSpan(input2);

   ASSERT_NE(key0, key1);

   std::vector<const ftags::Record*> collector;

   newManager.forEachRecordWithSymbol(2, [&collector](const ftags::Record* record) { collector.push_back(record); });
   ASSERT_EQ(3, collector.size());

   std::vector<const ftags::Record*> filtered =
      newManager.filterRecordsWithSymbol(2, [](const ftags::Record* /* record */) { return true; });
   ASSERT_EQ(3, filtered.size());
}
