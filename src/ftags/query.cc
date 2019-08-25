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

struct ns_sep : pegtl::rep<2, pegtl::one<':'>>
{
};

template <typename Key>
struct key : pegtl::seq<Key, pegtl::not_at<pegtl::identifier_other>>
{
};

// clang-format off
struct str_find : TAO_PEGTL_STRING("find") {};

struct str_namespace: TAO_PEGTL_STRING("parameter") {};

struct str_symbol : TAO_PEGTL_STRING("symbol") {};
struct str_function : TAO_PEGTL_STRING("function") {};
struct str_class : TAO_PEGTL_STRING("class") {};
struct str_struct : TAO_PEGTL_STRING("struct") {};
struct str_method: TAO_PEGTL_STRING("method") {};
struct str_attribute: TAO_PEGTL_STRING("attribute") {};
struct str_parameter: TAO_PEGTL_STRING("parameter") {};

struct str_type : pegtl::sor<str_symbol, str_function, str_parameter, str_class, str_struct, str_attribute, str_method> {};

struct key_find: key<str_find> {};
struct key_type: key<str_type> {};

struct namespace_qual : pegtl::seq<pegtl::identifier, pegtl::one<':'>, pegtl::one<':'>>
{
};

struct symbol_name : pegtl::seq<pegtl::identifier>
{
};

// clang-format on
struct grammar : pegtl::must<key_find,
                             sep,
                             pegtl::opt<pegtl::seq<key_type, sep>>,
                             pegtl::opt<ns_sep>,
                             pegtl::star<namespace_qual>,
                             symbol_name,
                             pegtl::eof>
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
struct action<str_class>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Class;
   }
};

template <>
struct action<str_method>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Method;
   }
};

template <>
struct action<str_attribute>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Attribute;
   }
};

template <>
struct action<str_parameter>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Parameter;
   }
};

template <>
struct action<namespace_qual>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.nameSpace.push_back(in.string());
   }
};

template <>
struct action<symbol_name>
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
