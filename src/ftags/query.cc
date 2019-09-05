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

#include <fmt/format.h>

#include <sstream>

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
struct str_identify: TAO_PEGTL_STRING("identify") {};
struct str_list: TAO_PEGTL_STRING("list") {};
struct str_ping: TAO_PEGTL_STRING("ping") {};
struct str_shutdown: TAO_PEGTL_STRING("shutdown") {};
struct str_dump: TAO_PEGTL_STRING("dump") {};
struct str_analyze: TAO_PEGTL_STRING("analyze") {};

struct str_projects: TAO_PEGTL_STRING("projects") {};
struct str_dependencies: TAO_PEGTL_STRING("dependencies") {};

struct str_at: TAO_PEGTL_STRING("at") {};
struct str_of: TAO_PEGTL_STRING("of") {};

struct str_symbol : TAO_PEGTL_STRING("symbol") {};
struct str_function : TAO_PEGTL_STRING("function") {};
struct str_class : TAO_PEGTL_STRING("class") {};
struct str_struct : TAO_PEGTL_STRING("struct") {};
struct str_method: TAO_PEGTL_STRING("method") {};
struct str_attribute: TAO_PEGTL_STRING("attribute") {};
struct str_parameter: TAO_PEGTL_STRING("parameter") {};
struct str_variable: TAO_PEGTL_STRING("variable") {};
struct str_statistics : TAO_PEGTL_STRING("statistics") {};

struct str_callers: TAO_PEGTL_STRING("callers") {};
struct str_containers: TAO_PEGTL_STRING("containers") {};
struct str_override: TAO_PEGTL_STRING("override") {};

struct str_declaration: TAO_PEGTL_STRING("declaration") {};
struct str_definition: TAO_PEGTL_STRING("definition") {};
struct str_reference: TAO_PEGTL_STRING("reference") {};
struct str_instantiation: TAO_PEGTL_STRING("instantiation") {};
struct str_destruction: TAO_PEGTL_STRING("destruction") {};

struct str_type : pegtl::sor<str_symbol, str_function, str_parameter, str_class, str_struct, str_attribute, str_method, str_variable> {};
struct str_qualifier : pegtl::sor<str_declaration, str_definition, str_reference, str_instantiation, str_destruction> {};

struct key_find: key<str_find> {};
struct key_identify: key<str_identify> {};
struct key_list: key<str_list> {};
struct key_ping: key<str_ping> {};
struct key_shutdown: key<str_shutdown> {};
struct key_dump: key<str_dump> {};
struct key_analyze: key<str_analyze> {};

struct key_override: key<str_override> {};

struct key_symbol: key<str_symbol> {};
struct key_type: key<str_type> {};
struct key_qualifier: key<str_qualifier> {};

struct key_projects: key<str_projects> {};
struct key_dependencies: key<str_dependencies> {};

struct namespace_qual : pegtl::seq<pegtl::identifier, pegtl::one<':'>, pegtl::one<':'>>
{
};

struct symbol_name : pegtl::seq<pegtl::identifier>
{
};

struct path_element : pegtl::star<pegtl::sor<pegtl::alnum, pegtl::one<'.', '-', '_', '+'>>>
{
};

// clang-format on

struct find_symbol
   : pegtl::if_must<
        key_find,
        sep,
        pegtl::sor<pegtl::if_must<key_override, sep, str_of, sep>,
                   pegtl::seq<pegtl::opt<pegtl::seq<key_type, sep>>, pegtl::opt<pegtl::seq<key_qualifier, sep>>>>,
        pegtl::opt<ns_sep>,
        pegtl::star<namespace_qual>,
        symbol_name,
        pegtl::eof>
{
};

// clang-format off
struct path_rootless : pegtl::seq<path_element, pegtl::star<pegtl::one<'/'>, path_element>>
{
};

struct path_absolute : pegtl::seq<pegtl::one<'/'>, path_rootless>
{
};

struct path : pegtl::sor<path_rootless, path_absolute>
{
};
// clang-format on

struct line_number : pegtl::plus<pegtl::digit>
{
};

struct column_number : pegtl::plus<pegtl::digit>
{
};

struct location : pegtl::seq<path, pegtl::one<':'>, line_number, pegtl::one<':'>, column_number>
{
};

struct identify_symbol : pegtl::if_must<key_identify, sep, key_symbol, sep, str_at, sep, location, pegtl::eof>
{
};

struct list_projects
   : pegtl::if_must<key_list,
                    sep,
                    pegtl::sor<key_projects, pegtl::if_must<key_dependencies, sep, str_of, sep, path>>,
                    pegtl::eof>
{
};

struct ping_server : pegtl::if_must<key_ping, pegtl::eof>
{
};

struct shutdown_server : pegtl::if_must<key_shutdown, pegtl::eof>
{
};

struct dump_stats : pegtl::if_must<key_dump, sep, symbol_name, sep, str_statistics, pegtl::eof>
{
};

struct analyze_data : pegtl::if_must<key_analyze, sep, symbol_name, pegtl::eof>
{
};

struct grammar
   : pegtl::sor<find_symbol, identify_symbol, list_projects, ping_server, shutdown_server, dump_stats, analyze_data>
{
};

template <typename Rule>
struct action
{
};

template <>
struct action<key_find>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::Find;
   }
};

template <>
struct action<key_identify>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::Identify;
   }
};

template <>
struct action<key_list>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::List;
   }
};

template <>
struct action<key_ping>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::Ping;
   }
};

template <>
struct action<key_shutdown>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::Shutdown;
   }
};

template <>
struct action<key_dump>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::Dump;
   }
};

template <>
struct action<key_analyze>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.verb = ftags::query::Query::Verb::Analyze;
   }
};

template <>
struct action<str_function>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Function;
   }
};

template <>
struct action<str_variable>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Variable;
   }
};

template <>
struct action<str_override>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Override;
   }
};

template <>
struct action<str_class>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Class;
   }
};

template <>
struct action<str_statistics>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Statistics;
   }
};

template <>
struct action<str_method>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Method;
   }
};

template <>
struct action<str_attribute>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Attribute;
   }
};

template <>
struct action<str_parameter>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Parameter;
   }
};

template <>
struct action<ns_sep>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.inGlobalNamespace = true;
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

template <>
struct action<path>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.filePath = in.string();
   }
};

template <>
struct action<line_number>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.lineNumber = static_cast<unsigned>(std::stoul(in.string()));
   }
};

template <>
struct action<column_number>
{
   template <typename Input>
   static void apply(const Input& in, ftags::query::Query& query)
   {
      query.columnNumber = static_cast<unsigned>(std::stoul(in.string()));
   }
};

template <>
struct action<str_reference>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.qualifier = ftags::query::Query::Qualifier::Reference;
   }
};

template <>
struct action<str_declaration>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.qualifier = ftags::query::Query::Qualifier::Declaration;
   }
};

template <>
struct action<str_definition>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.qualifier = ftags::query::Query::Qualifier::Definition;
   }
};

template <>
struct action<str_projects>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Project;
   }
};

template <>
struct action<str_dependencies>
{
   template <typename Input>
   static void apply(const Input& /* in */, ftags::query::Query& query)
   {
      query.type = ftags::query::Query::Type::Dependency;
   }
};

} // anonymous namespace

ftags::query::Query::Query(std::string_view input)
{
   pegtl::memory_input in(input.data(), input.size(), "");

   pegtl::parse<grammar, action>(in, *this);
}

ftags::query::Query ftags::query::Query::parse(std::string_view input)
{
   try
   {
      ftags::query::Query query(input);

      return query;
   }
   catch (const tao::pegtl::parse_error& parseError)
   {
      throw std::runtime_error(fmt::format("Failed to parse input: {}", parseError.std::exception::what()));
   }
   catch (const tao::pegtl::input_error& inputError)
   {
      throw std::runtime_error(fmt::format("Failed to parse input: {}", inputError.std::exception::what()));
   }
}

ftags::query::Query ftags::query::Query::parse(std::vector<std::string> input)
{
   std::stringstream os;

   bool first = true;

   for (const auto& elem : input)
   {
      if (!first)
      {
         os << ' ';
      }

      os << elem;
      first = false;
   }

   return parse(os.str());
}
