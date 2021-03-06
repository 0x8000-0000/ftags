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

#ifndef FTAGS_QUERY_H_INCLUDED
#define FTAGS_QUERY_H_INCLUDED

#include <string>
#include <string_view>
#include <vector>

namespace ftags::query
{

struct Query
{
   enum Verb : uint8_t
   {
      Unknown,
      Find,
      List,
      Identify,
      Ping,
      Shutdown,
      Dump,
      Analyze,
      Load,
      Save,
   };

   enum Type : uint8_t
   {
      Symbol,
      Function,
      Class,
      Structure,
      Union,
      Method,
      Attribute,
      Parameter,
      Override,
      Variable,
      Project,
      Dependency,
      Statistics,
      Contents,
   };

   enum Qualifier : uint8_t
   {
      Any,
      Declaration,
      Definition,
      Reference,
      Instantiation,
      Destruction,
   };

   enum Verb      verb      = Unknown;
   enum Type      type      = Symbol;
   enum Qualifier qualifier = Any;

   bool inGlobalNamespace = false;

   std::string              symbolName;
   std::vector<std::string> nameSpace;

   std::string translationUnit;
   std::string pathFragment;

   std::string filePath;
   unsigned    lineNumber   = 0;
   unsigned    columnNumber = 0;

   Query() = default;
   explicit Query(std::string_view input);

   static Query parse(std::string_view input);
   static Query parse(std::vector<std::string> input);
};

} // namespace ftags::query

#endif // FTAGS_QUERY_H_INCLUDED
