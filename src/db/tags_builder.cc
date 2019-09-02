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

#ifdef DUMP_SKIPPED_CURSORS
#include <iostream>
#endif

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

static void getSymbolType(CXCursor clangCursor, ftags::Attributes& attributes)
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
      attributes.isReference  = true;
      break;

#if 0
      /*
       * do not index literals
       */

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
#endif

   case CXCursor_DeclRefExpr:
      symbolType               = ftags::SymbolType::DeclarationReferenceExpression;
      attributes.isDeclaration = true;
      attributes.isReference   = true;
      attributes.isExpression  = true;
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
#ifdef DUMP_SKIPPED_CURSORS
      std::cerr << "Ignoring " << cursorKind << std::endl;
#endif
      break;
   }

   attributes.setType(symbolType);
}

struct TranslationUnitAccumulator
{
   ftags::ProjectDb::TranslationUnit& translationUnit;
   ftags::util::StringTable&          symbolTable;
   ftags::util::StringTable&          namespaceTable;
   ftags::util::StringTable&          fileNameTable;
   ftags::RecordSpanManager&          recordSpanManager;
   std::string                        filterPath;

   std::map<std::string, ftags::util::StringTable::Key> fileKeyCache;

   TranslationUnitAccumulator(ftags::ProjectDb::TranslationUnit&                translationUnit_,
                              ftags::ProjectDb::TranslationUnit::ParsingContext parsingContext) :
      translationUnit{translationUnit_},
      symbolTable{parsingContext.symbolTable},
      namespaceTable{parsingContext.namespaceTable},
      fileNameTable{parsingContext.fileNameTable},
      recordSpanManager{parsingContext.recordSpanManager},
      filterPath{parsingContext.filterPath}
   {
   }

   void processCursor(CXCursor clangCursor);

   bool getCursorLocation(CXCursor                       clangCursor,
                          ftags::Cursor::Location&       cursorLocation,
                          ftags::util::StringTable::Key* fileNameKey);
};

bool TranslationUnitAccumulator::getCursorLocation(CXCursor                       clangCursor,
                                                   ftags::Cursor::Location&       cursorLocation,
                                                   ftags::util::StringTable::Key* fileNameKey)
{
   CXStringWrapper fileNameWrapper;

   CXSourceLocation location = clang_getCursorLocation(clangCursor);
   clang_getPresumedLocation(location, fileNameWrapper.get(), &cursorLocation.line, &cursorLocation.column);

   std::string fileName{fileNameWrapper.c_str()};
   const auto  iter = fileKeyCache.find(fileName);

   if (iter != fileKeyCache.end())
   {
      *fileNameKey = iter->second;
   }
   else
   {
      std::filesystem::path filePath{fileName};
      if (std::filesystem::exists(filePath))
      {
         std::filesystem::path canonicalFilePath = std::filesystem::canonical(filePath);
         *fileNameKey                            = fileNameTable.addKey(canonicalFilePath.string().data());
         const auto insertIter                   = fileKeyCache.emplace(std::move(fileName), *fileNameKey);
#ifdef NDEBUG
         (void)insertIter;
#endif
         assert(insertIter.second);
      }
      else
      {
         std::filesystem::path otherPath = std::filesystem::current_path() / filePath;
         if (std::filesystem::exists(otherPath))
         {
            std::filesystem::path canonicalFilePath = std::filesystem::canonical(otherPath);
            *fileNameKey                            = fileNameTable.addKey(canonicalFilePath.string().data());
            const auto insertIter                   = fileKeyCache.emplace(std::move(fileName), *fileNameKey);
#ifdef NDEBUG
            (void)insertIter;
#endif
            assert(insertIter.second);
         }
         else
         {
            if (fileName == "<built-in>")
            {
               *fileNameKey          = fileNameTable.addKey(fileName.data());
               const auto insertIter = fileKeyCache.emplace(std::move(fileName), *fileNameKey);
#ifdef NDEBUG
               (void)insertIter;
#endif
               assert(insertIter.second);
            }
            else
            {
               assert(false);
            }
         }
      }

      cursorLocation.fileName = nullptr;
   }

   return clang_Location_isFromMainFile(location);
}

void TranslationUnitAccumulator::processCursor(CXCursor clangCursor)
{
   // check if the cursor is defined in a file below filterPath and if not, bail out early
   if (!filterPath.empty())
   {
      CXStringWrapper fileName;

      CXSourceLocation location = clang_getCursorLocation(clangCursor);
      clang_getPresumedLocation(location, fileName.get(), nullptr, nullptr);

      const std::size_t thisPathLength = strlen(fileName.c_str());

      if (thisPathLength < filterPath.size())
      {
         return;
      }

      if (memcmp(fileName.c_str(), filterPath.data(), filterPath.size()) != 0)
      {
         return;
      }
   }

   ftags::Cursor cursor = {};
   // get it early to aid debugging
   CXStringWrapper name{clang_getCursorSpelling(clangCursor)};
   cursor.symbolName = name.c_str();

   getSymbolType(clangCursor, cursor.attributes);

   if (cursor.attributes.getType() == ftags::SymbolType::Undefined)
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

#if 0
   const CXSourceRange    sourceRange = clang_getCursorExtent(clangCursor);
   const CXSourceLocation rangeStart  = clang_getRangeStart(sourceRange);
   const CXSourceLocation rangeEnd    = clang_getRangeEnd(sourceRange);
   // TODO: combine FunctionCallExpression with the subsequent DeclarationReferenceExpression and optional NamespaceReference
#endif

   ftags::util::StringTable::Key fileNameKey = 0;
   cursor.attributes.isFromMainFile          = getCursorLocation(clangCursor, cursor.location, &fileNameKey);
   assert(fileNameKey != 0);

   if (clang_isCursorDefinition(clangCursor))
   {
      cursor.attributes.isDefinition = 1;
   }

   CXStringWrapper unifiedSymbol{clang_getCursorUSR(clangCursor)};

   cursor.unifiedSymbol = unifiedSymbol.c_str();

   CXCursor                      referencedCursor      = clang_getCursorReferenced(clangCursor);
   ftags::util::StringTable::Key referencedFileNameKey = 0;
   cursor.attributes.isDefinedInMainFile =
      getCursorLocation(referencedCursor, cursor.definition, &referencedFileNameKey);
   assert(referencedFileNameKey != 0);

   const ftags::util::StringTable::Key symbolNameKey = symbolTable.addKey(cursor.symbolName);

   translationUnit.addCursor(cursor, symbolNameKey, fileNameKey, referencedFileNameKey, recordSpanManager);
}

CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   TranslationUnitAccumulator* accumulator = reinterpret_cast<TranslationUnitAccumulator*>(clientData);

   accumulator->processCursor(cursor);

   clang_visitChildren(cursor, visitTranslationUnit, clientData);
   return CXChildVisit_Continue;
}

} // namespace

ftags::ProjectDb::TranslationUnit ftags::ProjectDb::TranslationUnit::parse(const std::string&              fileName,
                                                                           const std::vector<const char*>& arguments,
                                                                           ParsingContext& parsingContext)
{
   ftags::ProjectDb::TranslationUnit translationUnit;

   TranslationUnitAccumulator accumulator{translationUnit, parsingContext};

   ftags::util::StringTable::Key fileKey = parsingContext.fileNameTable.addKey(fileName.c_str());
   translationUnit.beginParsingUnit(fileKey);

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
      clang_visitChildren(cursor, visitTranslationUnit, &accumulator);
   }
   else
   {
      throw std::runtime_error("Failed to parse input");
   }

   translationUnit.finalizeParsingUnit(parsingContext.recordSpanManager);

   return translationUnit;
}

/*
 * Use clang_getCursorReferenced to get to declaration
 */
