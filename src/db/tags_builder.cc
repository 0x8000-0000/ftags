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

#include <filesystem>
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

static ftags::SymbolType getSymbolType(CXCursor clangCursor, ftags::Attributes& attributes)
{
   enum CXCursorKind cursorKind = clang_getCursorKind(clangCursor);

   ftags::SymbolType symbolType = ftags::SymbolType::Undefined;

   switch (cursorKind)
   {
   case CXCursor_FieldDecl:
      symbolType               = ftags::SymbolType::FieldDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_EnumConstantDecl:
      symbolType               = ftags::SymbolType::EnumerationConstantDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_UnionDecl:
      symbolType               = ftags::SymbolType::UnionDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_StructDecl:
      symbolType               = ftags::SymbolType::StructDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_ClassDecl:
      symbolType               = ftags::SymbolType::ClassDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_EnumDecl:
      symbolType               = ftags::SymbolType::EnumerationDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_FunctionDecl:
      symbolType               = ftags::SymbolType::FunctionDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_VarDecl:
      symbolType               = ftags::SymbolType::VariableDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_ParmDecl:
      symbolType               = ftags::SymbolType::ParameterDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_TypedefDecl:
      symbolType               = ftags::SymbolType::TypedefDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_CXXMethod:
      symbolType               = ftags::SymbolType::MethodDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_Namespace:
      symbolType               = ftags::SymbolType::Namespace;
      attributes.isDeclaration = true;
      break;

   case CXCursor_Constructor:
      symbolType               = ftags::SymbolType::Constructor;
      attributes.isDeclaration = true;
      break;

   case CXCursor_Destructor:
      symbolType               = ftags::SymbolType::Destructor;
      attributes.isDeclaration = true;
      break;

   case CXCursor_NonTypeTemplateParameter:
      symbolType = ftags::SymbolType::NonTypeTemplateParameter;
      break;

   case CXCursor_TemplateTypeParameter:
      symbolType               = ftags::SymbolType::TemplateTypeParameter;
      attributes.isDeclaration = true;
      break;

   case CXCursor_FunctionTemplate:
      symbolType               = ftags::SymbolType::FunctionTemplate;
      attributes.isDeclaration = true;
      break;

   case CXCursor_UsingDeclaration:
      symbolType               = ftags::SymbolType::UsingDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_TypeAliasDecl:
      symbolType               = ftags::SymbolType::TypeAliasDeclaration;
      attributes.isDeclaration = true;
      break;

   case CXCursor_CXXBaseSpecifier:
      symbolType = ftags::SymbolType::BaseSpecifier;
      break;

   case CXCursor_TypeRef:
      symbolType = ftags::SymbolType::TypeReference;
      break;

   case CXCursor_TemplateRef:
      symbolType = ftags::SymbolType::TemplateReference;
      break;

   case CXCursor_ClassTemplate:
      symbolType = ftags::SymbolType::ClassTemplate;
      break;

   case CXCursor_ClassTemplatePartialSpecialization:
      symbolType = ftags::SymbolType::ClassTemplatePartialSpecialization;
      break;

   case CXCursor_NamespaceAlias:
      symbolType = ftags::SymbolType::NamespaceAlias;
      break;

   case CXCursor_NamespaceRef:
      symbolType                = ftags::SymbolType::NamespaceReference;
      attributes.isNamespaceRef = true;
      break;

   case CXCursor_MemberRef:
      symbolType             = ftags::SymbolType::MemberReference;
      attributes.isReference = true;
      break;

   case CXCursor_VariableRef:
      symbolType             = ftags::SymbolType::VariableReference;
      attributes.isReference = true;
      break;

   case CXCursor_OverloadedDeclRef:
      symbolType             = ftags::SymbolType::OverloadedDeclarationReference;
      attributes.isReference = true;
      break;

#if 0
   case CXCursor_UnexposedExpr:
      symbolType              = ftags::SymbolType::UnexposedExpression;
      attributes.isExpression = true;
      break;
#endif

   case CXCursor_CallExpr:
      symbolType              = ftags::SymbolType::FunctionCallExpression;
      attributes.isExpression = true;
      break;

   case CXCursor_IntegerLiteral:
      symbolType = ftags::SymbolType::IntegerLiteral;
      break;

   case CXCursor_StringLiteral:
      symbolType = ftags::SymbolType::StringLiteral;
      break;

   case CXCursor_CharacterLiteral:
      symbolType = ftags::SymbolType::CharacterLiteral;
      break;

   case CXCursor_FloatingLiteral:
      symbolType = ftags::SymbolType::FloatingLiteral;
      break;

   case CXCursor_DeclRefExpr:
      symbolType              = ftags::SymbolType::DeclarationReferenceExpression;
      attributes.isReference  = true;
      attributes.isExpression = true;
      break;

   case CXCursor_MemberRefExpr:
      symbolType              = ftags::SymbolType::MemberReferenceExpression;
      attributes.isReference  = true;
      attributes.isExpression = true;
      break;

   case CXCursor_CStyleCastExpr:
      symbolType        = ftags::SymbolType::CStyleCastExpression;
      attributes.isCast = true;
      break;

   case CXCursor_MacroDefinition:
      symbolType = ftags::SymbolType::MacroDefinition;
      break;

   case CXCursor_MacroExpansion:
      symbolType = ftags::SymbolType::MacroExpansion;
      break;

   case CXCursor_InclusionDirective:
      symbolType = ftags::SymbolType::InclusionDirective;
      break;

   case CXCursor_TypeAliasTemplateDecl:
      symbolType               = ftags::SymbolType::TypeAliasTemplateDecl;
      attributes.isDeclaration = true;
      break;

   default:
      break;
   }

   return symbolType;
}

void processCursor(ftags::TranslationUnit* translationUnit, CXCursor clangCursor)
{
   ftags::Cursor cursor = {};
   // get it early to aid debugging
   CXStringWrapper name{clang_getCursorSpelling(clangCursor)};
   cursor.symbolName = name.c_str();

   ftags::Attributes attributes = {};
   cursor.symbolType            = getSymbolType(clangCursor, attributes);

   if (cursor.symbolType == ftags::SymbolType::Undefined)
   {
#if 0
      enum CXCursorKind cursorKind = clang_getCursorKind(clangCursor);
      if (CXCursor_FirstExpr != cursorKind)
      {
         std::cerr << "@@ Unhandled symbol " << cursor.symbolName << " code: " << cursorKind << std::endl;
      }
#endif

      // don't know how to handle this cursor; just ignore it
      return;
   }

   CXStringWrapper fileName;
   unsigned int    line   = 0;
   unsigned int    column = 0;

   CXSourceLocation location = clang_getCursorLocation(clangCursor);
   clang_getPresumedLocation(location, fileName.get(), &line, &column);

   if (clang_Location_isFromMainFile(location))
   {
      attributes.isFromMainFile = 1;
   }

   if (clang_isCursorDefinition(clangCursor))
   {
      attributes.isDefinition = 1;
   }

   const char*           fileNameAsRawCString = fileName.c_str();
   std::filesystem::path filePath{fileNameAsRawCString};
   std::string           canonicalFilePathAsString{fileNameAsRawCString};
   if (std::filesystem::exists(filePath))
   {
      std::filesystem::path canonicalFilePath = std::filesystem::canonical(filePath);
      canonicalFilePathAsString               = canonicalFilePath.string();
   }
   else
   {
      std::filesystem::path otherPath = std::filesystem::current_path() / filePath;
      if (std::filesystem::exists(otherPath))
      {
         std::filesystem::path canonicalFilePath = std::filesystem::canonical(otherPath);
         canonicalFilePathAsString               = canonicalFilePath.string();
      }
   }

   cursor.location.fileName = canonicalFilePathAsString.data();
   cursor.location.line     = static_cast<int>(line);
   cursor.location.column   = static_cast<int>(column);

   translationUnit->addCursor(cursor, attributes);
}

CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   ftags::TranslationUnit* translationUnit = reinterpret_cast<ftags::TranslationUnit*>(clientData);

   processCursor(translationUnit, cursor);

   clang_visitChildren(cursor, visitTranslationUnit, clientData);
   return CXChildVisit_Continue;
}

} // namespace

ftags::TranslationUnit ftags::TranslationUnit::parse(const std::string&       fileName,
                                                     std::vector<const char*> arguments,
                                                     StringTable&             symbolTable,
                                                     StringTable&             fileNameTable)
{
   ftags::TranslationUnit translationUnit(symbolTable, fileNameTable);

   auto clangIndex = std::unique_ptr<void, CXIndexDestroyer>(clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                                                               /* displayDiagnostics         = */ 0));

   CXTranslationUnit translationUnitPtr = nullptr;

   const unsigned parsingOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_KeepGoing |
                                   CXTranslationUnit_CreatePreambleOnFirstParse |
                                   clang_defaultEditingTranslationUnitOptions();

   const CXErrorCode parseError = clang_parseTranslationUnit2(
      /* CIdx                  = */ clangIndex.get(),
      /* source_filename       = */ fileName.c_str(),
      /* command_line_args     = */ arguments.data(),
      /* num_command_line_args = */ static_cast<int>(arguments.size()),
      /* unsaved_files         = */ nullptr,
      /* num_unsaved_files     = */ 0,
      /* options               = */ parsingOptions,
      /* out_TU                = */ &translationUnitPtr);

   if ((parseError == CXError_Success) && (nullptr != translationUnitPtr))
   {
      CXDiagnosticSet diagnosticSet    = clang_getDiagnosticSetFromTU(translationUnitPtr);
      unsigned        diagnosticsCount = clang_getNumDiagnosticsInSet(diagnosticSet);

      if (diagnosticsCount)
      {
         const unsigned defaultDisplayOptions = clang_defaultDiagnosticDisplayOptions();
         for (unsigned ii = 0; ii < diagnosticsCount; ii++)
         {
            CXDiagnostic diagnostic = clang_getDiagnosticInSet(diagnosticSet, ii);

            CXString message = clang_formatDiagnostic(diagnostic, defaultDisplayOptions);

            const char* msgStr = clang_getCString(message);
            (void)msgStr;

            clang_disposeString(message);
            clang_disposeDiagnostic(diagnostic);
         }
      }

      clang_disposeDiagnosticSet(diagnosticSet);

      auto clangTranslationUnit =
         std::unique_ptr<CXTranslationUnitImpl, CXTranslationUnitDestroyer>(translationUnitPtr);

      CXCursor cursor = clang_getTranslationUnitCursor(clangTranslationUnit.get());
      clang_visitChildren(cursor, visitTranslationUnit, &translationUnit);
   }
   else
   {
      throw std::runtime_error("Failed to parse input");
   }

   return translationUnit;
}

/*
 * Use clang_getCursorReferenced to get to declaration
 */
