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

#ifndef FILE_NAME_TABLE_H_INCLUDED
#define FILE_NAME_TABLE_H_INCLUDED

#include <serialization.h>

#include <store.h>
#include <string_table.h>

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace ftags::util
{

std::vector<std::string_view> splitPath(std::string_view pathInput);

class FileNameTable
{
public:
   FileNameTable()
   {
      m_parentToElement.push_back(PathElement());
   }

   using Key = uint32_t;

   static constexpr Key InvalidKey = 0;

   const std::string getPath(Key pathKey) const noexcept;

   Key  getKey(std::string_view path) const noexcept;
   Key  addKey(std::string_view path);
   void removeKey(std::string_view path);

   /* Add all the keys from other that are missing in this table.
    *
    * @return a mapping between the keys in the old table and the keys in the updated table
    */
   FlatMap<FileNameTable::Key, FileNameTable::Key> mergeStringTable(const FileNameTable& other);

   /*
    * Serialization interface
    */
   std::size_t computeSerializedSize() const;

   void serialize(BufferInsertor& insertor) const;

   static StringTable deserialize(BufferExtractor& extractor);

private:
   struct PathElement
   {
      StringTable::Key   pathElementKey = StringTable::k_InvalidKey;
      FileNameTable::Key parentPathKey  = FileNameTable::InvalidKey;

      bool operator<(const PathElement& other) const
      {
         if (pathElementKey < other.pathElementKey)
         {
            return true;
         }

         if (pathElementKey == other.parentPathKey)
         {
            if (parentPathKey < other.parentPathKey)
            {
               return true;
            }
         }

         return false;
      }

      bool operator==(const PathElement& other) const
      {
         return (pathElementKey == other.pathElementKey) && (parentPathKey < other.parentPathKey);
      }
   };

   struct SharedPathElement : public PathElement
   {
      uint32_t referenceCount : 31;
      uint32_t isTerminal : 1;

      SharedPathElement(const PathElement& pathElement) : PathElement{pathElement}, referenceCount{1}, isTerminal{0}
      {
      }
   };

   /* persistent data
    */
   StringTable                    m_pathElements;
   std::vector<SharedPathElement> m_parentToElement;

   /* transient data
    */
   std::map<PathElement, FileNameTable::Key> m_elementToParent;
};

} // namespace ftags::util

#endif // FILE_NAME_TABLE_H_INCLUDED
