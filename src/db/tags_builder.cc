#include <tags_builder.h>

ftags::ProjectDb ftags::parseTranslationUnit(const std::string& /* fileName */, std::vector<const char*> /* arguments */)
{
   ftags::ProjectDb tags;
   return tags;
}

/*
 * Use clang_getCursorReferenced to get to declaration
 */
