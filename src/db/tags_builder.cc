#include <tags_builder.h>

ftags::Tags ftags::parseTranslationUnit(const std::string& /* fileName */, std::vector<const char*> /* arguments */)
{
   ftags::Tags tags;
   return tags;
}

/*
 * Use clang_getCursorReferenced to get to declaration
 */
