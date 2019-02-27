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

#include <project.h>

#include <clang-c/CXString.h>
#include <clang-c/Index.h>

#include <memory>
#include <stdexcept>

namespace
{

struct CXIndexDestroyer
{
   void operator()(void* obj) const noexcept
   {
      clang_disposeIndex(obj);
   }
};

struct CXTranslationUnitDestroyer
{
   void operator()(CXTranslationUnit translationUnit) const noexcept
   {
      clang_disposeTranslationUnit(translationUnit);
   }
};

class CXStringWrapper
{
public:
   CXStringWrapper()
   {
   }

   CXStringWrapper(CXString cxString) : m_string(cxString)
   {
   }

   ~CXStringWrapper() noexcept
   {
      clang_disposeString(m_string);
   }

   const char* c_str() const noexcept
   {
      return clang_getCString(m_string);
   }

   CXString* get() noexcept
   {
      return &m_string;
   }

private:
   CXString m_string;
};

void processCursor(ftags::ProjectDb* projectDb, CXCursor clangCursor)
{
   CXStringWrapper name{clang_getCursorSpelling(clangCursor)};

   ftags::Cursor     cursor;
   ftags::Attributes attributes;

   cursor.symbolName = name.c_str();
   cursor.symbolType = static_cast<ftags::SymbolType>(clang_getCursorKind(clangCursor));

   CXStringWrapper fileName;
   unsigned int    line   = 0;
   unsigned int    column = 0;

   CXSourceLocation location = clang_getCursorLocation(clangCursor);
   clang_getPresumedLocation(location, fileName.get(), &line, &column);

   cursor.location.fileName = fileName.c_str();
   cursor.location.line     = static_cast<int>(line);
   cursor.location.column   = static_cast<int>(column);

   projectDb->addCursor(cursor, attributes);
}

CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   ftags::ProjectDb* projectDb = reinterpret_cast<ftags::ProjectDb*>(clientData);

   processCursor(projectDb, cursor);

   clang_visitChildren(cursor, visitTranslationUnit, clientData);
   return CXChildVisit_Continue;
}

} // namespace

ftags::ProjectDb ftags::ProjectDb::parseTranslationUnit(const std::string& fileName, std::vector<const char*> arguments)
{
   ftags::ProjectDb tags;

   auto clangIndex = std::unique_ptr<void, CXIndexDestroyer>(clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                                                               /* displayDiagnostics         = */ 0));

   CXTranslationUnit translationUnitPtr = nullptr;

   const CXErrorCode parseError = clang_parseTranslationUnit2(
      /* CIdx                  = */ clangIndex.get(),
      /* source_filename       = */ fileName.c_str(),
      /* command_line_args     = */ arguments.data(),
      /* num_command_line_args = */ static_cast<int>(arguments.size()),
      /* unsaved_files         = */ nullptr,
      /* num_unsaved_files     = */ 0,
      /* options               = */ CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_SingleFileParse,
      /* out_TU                = */ &translationUnitPtr);

   if ((parseError == CXError_Success) && (nullptr != translationUnitPtr))
   {
      auto translationUnit = std::unique_ptr<CXTranslationUnitImpl, CXTranslationUnitDestroyer>(translationUnitPtr);

      CXCursor cursor = clang_getTranslationUnitCursor(translationUnit.get());
      clang_visitChildren(cursor, visitTranslationUnit, &tags);
   }
   else
   {
      throw std::runtime_error("Failed to parse input");
   }

   return tags;
}

/*
 * Use clang_getCursorReferenced to get to declaration
 */
