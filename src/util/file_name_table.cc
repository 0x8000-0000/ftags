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

#include <algorithm>
#include <list>
#include <sstream>
#include <vector>

/*
 * adapted from https://github.com/fenbf/StringViewTests/blob/master/StringViewTest.cpp
 */
std::vector<std::string_view> ftags::util::splitPath(std::string_view pathInput)
{
   std::vector<std::string_view> result;

   auto first  = pathInput.data();
   auto second = pathInput.data();
   auto last   = first + pathInput.size();

   if (*first == '/')
   {
      result.emplace_back(first, 0);
      first++;
      second++;
   }

   while (second != last && first != last)
   {
      second = std::find(first, last, '/');

      if (first != second)
      {
         result.emplace_back(first, second - first);
      }

      first = second + 1;
   }

   return result;
}

const std::string ftags::util::FileNameTable::getPath(ftags::util::FileNameTable::Key pathKey) const noexcept
{
   std::list<const char*> elements;

   while (pathKey != InvalidKey)
   {
      assert(pathKey < m_parentToElement.size());

      elements.push_back(m_pathElements.getString(m_parentToElement[pathKey].pathElementKey));
      pathKey = m_parentToElement[pathKey].parentPathKey;
   }

   elements.reverse();

   std::ostringstream os;

   bool addSeparator = false;

   for (const auto elem : elements)
   {
      if (addSeparator)
      {
         os << '/';
      }
      else
      {
         addSeparator = true;
      }

      os << elem;
   }

   return os.str();
}

ftags::util::FileNameTable::Key ftags::util::FileNameTable::getKey(std::string_view path) const noexcept
{
   const std::vector<std::string_view> elements = splitPath(path);

   FileNameTable::Key currentPathKey = InvalidKey;

   for (const auto& elem : elements)
   {
      StringTable::Key elemKey = m_pathElements.getKey(elem);

      if (elemKey == StringTable::k_InvalidKey)
      {
         return InvalidKey;
      }

      PathElement thisElement{elemKey, currentPathKey};

      auto iter = m_elementToParent.find(thisElement);

      if (iter == m_elementToParent.end())
      {
         return InvalidKey;
      }
      else
      {
         currentPathKey = iter->second;
      }
   }

   if (m_parentToElement[currentPathKey].isTerminal == 1)
   {
      return currentPathKey;
   }
   else
   {
      return InvalidKey;
   }
}

ftags::util::FileNameTable::Key ftags::util::FileNameTable::addKey(std::string_view path)
{
   const std::vector<std::string_view> elements = splitPath(path);

   FileNameTable::Key currentPathKey = InvalidKey;

   for (const auto& elem : elements)
   {
      StringTable::Key elemKey = m_pathElements.addKey(elem);

      PathElement thisElement{elemKey, currentPathKey};

      auto iter = m_elementToParent.find(thisElement);

      if (iter == m_elementToParent.end())
      {
         currentPathKey = static_cast<Key>(m_parentToElement.size());
         m_parentToElement.push_back(thisElement);
         m_elementToParent.emplace(thisElement, currentPathKey);
      }
      else
      {
         currentPathKey = iter->second;
         m_parentToElement[currentPathKey].referenceCount++;
      }
   }

   m_parentToElement[currentPathKey].isTerminal = 1;

   return currentPathKey;
}

void ftags::util::FileNameTable::removeKey(std::string_view path)
{
   const std::vector<std::string_view> elements = splitPath(path);

   FileNameTable::Key currentPathKey = InvalidKey;

   for (const auto& elem : elements)
   {
      StringTable::Key elemKey = m_pathElements.getKey(elem);

      if (elemKey == StringTable::k_InvalidKey)
      {
         // this path element does not exist
         return;
      }

      PathElement thisElement{elemKey, currentPathKey};

      auto iter = m_elementToParent.find(thisElement);

      if (iter == m_elementToParent.end())
      {
         // this path element does not exist
         return;
      }
      else
      {
         currentPathKey = iter->second;
         m_parentToElement[currentPathKey].referenceCount--;
      }
   }

   m_parentToElement[currentPathKey].isTerminal = 0;
}
