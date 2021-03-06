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

#include <file_name_table.h>

#include <gtest/gtest.h>

using ftags::util::FileNameTable;

TEST(FileNameTableTest, EmptyTableHasNoString)
{
   FileNameTable fnt;

   const uint32_t keyForInvalidValue = fnt.getKey("foo");

   ASSERT_EQ(keyForInvalidValue, 0);
}

TEST(FileNameTableTest, SplitAbsolutePath)
{
   FileNameTable fnt;

   std::vector<std::string_view> elements = ftags::util::splitPath("/home/test/foo");

   ASSERT_EQ(elements.size(), 4);

   ASSERT_EQ(elements[0], "");
   ASSERT_EQ(elements[1], "home");
   ASSERT_EQ(elements[2], "test");
   ASSERT_EQ(elements[3], "foo");
}

TEST(FileNameTableTest, SplitRelativePath)
{
   FileNameTable fnt;

   std::vector<std::string_view> elements = ftags::util::splitPath("home/test/foo");

   ASSERT_EQ(elements.size(), 3);

   ASSERT_EQ(elements[0], "home");
   ASSERT_EQ(elements[1], "test");
   ASSERT_EQ(elements[2], "foo");
}

TEST(FileNameTableTest, SplitPathWithConsecutiveSeparators)
{
   FileNameTable fnt;

   std::vector<std::string_view> elements = ftags::util::splitPath("home///test/foo");

   ASSERT_EQ(elements.size(), 3);

   ASSERT_EQ(elements[0], "home");
   ASSERT_EQ(elements[1], "test");
   ASSERT_EQ(elements[2], "foo");
}

TEST(FileNameTableTest, AddOneAbsolutePathGetItBack)
{
   FileNameTable fnt;

   const FileNameTable::Key key = fnt.addKey("/home/test/foo");

   ASSERT_NE(key, FileNameTable::InvalidKey);

   std::string thePath = fnt.getPath(key);

   ASSERT_EQ(thePath, "/home/test/foo");
}

TEST(FileNameTableTest, AddOneRelativePathGetItBack)
{
   FileNameTable fnt;

   const FileNameTable::Key key = fnt.addKey("home/test/foo");
   ASSERT_NE(key, FileNameTable::InvalidKey);

   const std::string thePath = fnt.getPath(key);
   ASSERT_EQ(thePath, "home/test/foo");

   const FileNameTable::Key keyAgain = fnt.getKey("home/test/foo");
   ASSERT_EQ(keyAgain, key);
}

TEST(FileNameTableTest, RetrieveFullPathsOnly)
{
   FileNameTable fnt;

   const FileNameTable::Key key = fnt.addKey("home/test/foo");
   ASSERT_NE(key, FileNameTable::InvalidKey);

   const FileNameTable::Key partialKey = fnt.getKey("home/test");
   ASSERT_EQ(partialKey, FileNameTable::InvalidKey);
}

TEST(FileNameTableTest, IntermediaryPathsCanBecomeFullPaths)
{
   FileNameTable fnt;

   const FileNameTable::Key key = fnt.addKey("home/test/foo");
   ASSERT_NE(key, FileNameTable::InvalidKey);

   const FileNameTable::Key partialKey = fnt.addKey("home/test");
   ASSERT_NE(partialKey, FileNameTable::InvalidKey);

   const FileNameTable::Key partialKeyAgain = fnt.getKey("home/test");
   ASSERT_EQ(partialKeyAgain, partialKey);
}

TEST(FileNameTableTest, RemoveKeyOnlyKey)
{
   FileNameTable fnt;

   const FileNameTable::Key key = fnt.addKey("home/test/foo");
   ASSERT_NE(key, FileNameTable::InvalidKey);

   fnt.removeKey("home/test/foo");

   const FileNameTable::Key keyForRemovedEntry = fnt.getKey("home/test/foo");
   ASSERT_EQ(keyForRemovedEntry, FileNameTable::InvalidKey);

   const FileNameTable::Key keyForRemovedParentEntry = fnt.getKey("home/test");
   ASSERT_EQ(keyForRemovedParentEntry, FileNameTable::InvalidKey);
}

TEST(FileNameTableTest, RemoveOneKey)
{
   FileNameTable fnt;

   const FileNameTable::Key keyOne = fnt.addKey("home/test/foo");
   ASSERT_NE(keyOne, FileNameTable::InvalidKey);

   const FileNameTable::Key keyTwo = fnt.addKey("home/test/bar");
   ASSERT_NE(keyTwo, FileNameTable::InvalidKey);

   fnt.removeKey("home/test/foo");

   const FileNameTable::Key keyForRemovedEntry = fnt.getKey("home/test/foo");
   ASSERT_EQ(keyForRemovedEntry, FileNameTable::InvalidKey);

   const FileNameTable::Key keyForRemainingEntry = fnt.getKey("home/test/bar");
   ASSERT_EQ(keyForRemainingEntry, keyTwo);
}

TEST(FileNameTableTest, RemoveOneKeyThenAddItBack)
{
   FileNameTable fnt;

   const FileNameTable::Key keyOne = fnt.addKey("home/test/foo");
   ASSERT_NE(keyOne, FileNameTable::InvalidKey);

   const FileNameTable::Key keyTwo = fnt.addKey("home/test/bar");
   ASSERT_NE(keyTwo, FileNameTable::InvalidKey);

   fnt.removeKey("home/test/foo");

   const FileNameTable::Key keyForRemovedEntry = fnt.getKey("home/test/foo");
   ASSERT_EQ(keyForRemovedEntry, FileNameTable::InvalidKey);

   const FileNameTable::Key keyForRemainingEntry = fnt.getKey("home/test/bar");
   ASSERT_NE(keyForRemainingEntry, FileNameTable::InvalidKey);

   /*
    * adding back
    */

   const FileNameTable::Key keyOneBack = fnt.addKey("home/test/foo");
   ASSERT_NE(keyOneBack, FileNameTable::InvalidKey);

   const FileNameTable::Key keyForReaddedEntry = fnt.getKey("home/test/foo");
   ASSERT_NE(keyForReaddedEntry, FileNameTable::InvalidKey);

   const std::string remainingEntry = fnt.getPath(keyTwo);
   ASSERT_EQ(remainingEntry, "home/test/bar");

   const std::string readdedEntry = fnt.getPath(keyForReaddedEntry);
   ASSERT_EQ(readdedEntry, "home/test/foo");
}
