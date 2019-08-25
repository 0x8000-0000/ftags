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

struct sep : pegtl::plus<pegtl::ascii::space>
{
};

template <typename Key>
struct key : pegtl::seq<Key, pegtl::not_at<pegtl::identifier_other>>
{
};

// clang-format off
struct str_find : TAO_PEGTL_STRING("find") {};
struct str_symbol : TAO_PEGTL_STRING("symbol") {};
struct str_function : TAO_PEGTL_STRING("function") {};
struct str_class : TAO_PEGTL_STRING("class") {};
struct str_struct : TAO_PEGTL_STRING("struct") {};

struct str_type : pegtl::sor<str_symbol, str_class, str_function, str_struct> {};

struct key_find: key<str_find> {};
struct key_type: key<str_type> {};

// clang-format on
struct grammar : pegtl::must<key_find, sep, pegtl::opt<pegtl::seq<key_type, sep>>, pegtl::identifier, pegtl::eof>
{
};

template <typename Rule>
struct action
{
};

template <>
struct action<str_function>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Function;
   }
};

template <>
struct action<pegtl::identifier>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.symbolName = in.string();
   }
};

} // anonymous namespace

ftags::query::Query ftags::query::Query::parse(std::string_view input)
{
   ftags::query::Query query;

   pegtl::memory_input in(input.data(), input.size(), "");

   pegtl::parse<grammar, action>(in, query);

   return query;
}
