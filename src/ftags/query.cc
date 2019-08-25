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

#include <query.h>

#include <tao/pegtl.hpp>

namespace
{

namespace pegtl = tao::pegtl;

struct find : pegtl::string<'f', 'i', 'n', 'd'>
{
};

struct separator : pegtl::star<pegtl::ascii::space>
{
};

struct symbol : pegtl::plus<pegtl::alpha>
{
};

struct grammar : pegtl::must<find, separator, symbol, pegtl::eof>
{
};

template <typename Rule>
struct action
{
};

template <>
struct action<symbol>
{
   template <typename Input>
   static void apply(const Input& in, std::string& v)
   {
      v = in.string();
   }
};

} // anonymous namespace

ftags::query::Query ftags::query::Query::parse(std::string_view input)
{
   ftags::query::Query query;

   pegtl::memory_input in(input.data(), input.size(), "");

   pegtl::parse<grammar, action>(in, query.symbolName);

   return query;
}
