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

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <iostream>
#include <string>
#include <vector>

struct VisitData
{
   unsigned int level = 0;
};

static void dumpCursorInfo(CXCursor cursor, unsigned int level)
{
   CXString name = clang_getCursorSpelling(cursor);

   CXCursorKind kind     = clang_getCursorKind(cursor);
   CXString     kindName = clang_getCursorKindSpelling(kind);

   CXSourceLocation location = clang_getCursorLocation(cursor);

   CXString cursorUSR = clang_getCursorUSR(cursor);

   CXString     fileName;
   unsigned int line   = 0;
   unsigned int column = 0;

   clang_getPresumedLocation(location, &fileName, &line, &column);

   std::cout << clang_getCString(fileName) << ':' << line << ':' << column << ':' << std::string(level, ' ')
             << clang_getCString(kindName) << ':' << clang_getCString(name) << ':' << clang_getCString(cursorUSR)
             << std::endl;

   clang_disposeString(cursorUSR);
   clang_disposeString(fileName);
   clang_disposeString(kindName);
   clang_disposeString(name);
}

static CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   VisitData* visitData = reinterpret_cast<VisitData*>(clientData);

   dumpCursorInfo(cursor, visitData->level);

   VisitData childrenData;
   childrenData.level = visitData->level + 1;
   clang_visitChildren(cursor, visitTranslationUnit, &childrenData);

   return CXChildVisit_Continue;
}

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      return 1;
   }

   CXCompilationDatabase_Error ccderror            = CXCompilationDatabase_NoError;
   CXCompilationDatabase       compilationDatabase = clang_CompilationDatabase_fromDirectory(argv[1], &ccderror);

   if (CXCompilationDatabase_NoError == ccderror)
   {
      CXIndex index = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                        /* displayDiagnostics         = */ 0);

      CXCompileCommands compileCommands = clang_CompilationDatabase_getAllCompileCommands(compilationDatabase);

      const unsigned compilationCount = clang_CompileCommands_getSize(compileCommands);

      for (unsigned ii = 0; ii < compilationCount; ii++)
      {
         CXCompileCommand compileCommand = clang_CompileCommands_getCommand(compileCommands, ii);

         CXString    fileNameString = clang_CompileCommand_getFilename(compileCommand);
         const char* sourceFileName = clang_getCString(fileNameString);
         std::cout << "Processing " << sourceFileName << std::endl;

         const unsigned        argCount = clang_CompileCommand_getNumArgs(compileCommand);
         std::vector<CXString> argumentsAsCXString;
         argumentsAsCXString.reserve(argCount);
         std::vector<const char*> arguments;
         arguments.reserve(argCount);

         bool skipFileNames = false;

         for (unsigned jj = 0; jj < argCount; jj++)
         {
            if (skipFileNames)
            {
               skipFileNames = false;
               continue;
            }

            CXString cxString = clang_CompileCommand_getArg(compileCommand, jj);
            argumentsAsCXString.push_back(cxString);
            const char* argumentText = clang_getCString(cxString);

            if ((argumentText[0] == '-') && ((argumentText[1] == 'c') || (argumentText[1] == 'o')))
            {
               skipFileNames = true;
               continue;
            }

            arguments.push_back(argumentText);
            std::cout << "   " << arguments[jj] << std::endl;
         }

         CXTranslationUnit translationUnit = nullptr;

         const CXErrorCode parseError =
            clang_parseTranslationUnit2(/* CIdx                  = */ index,
                                        /* source_filename       = */ sourceFileName,
                                        /* command_line_args     = */ arguments.data(),
                                        /* num_command_line_args = */ static_cast<int>(arguments.size()),
                                        /* unsaved_files         = */ nullptr,
                                        /* num_unsaved_files     = */ 0,
                                        /* options               = */ CXTranslationUnit_DetailedPreprocessingRecord,
                                        /* out_TU                = */ &translationUnit);

         std::cout << "   Parse status: " << parseError << std::endl;
         if (parseError == CXError_Success)
         {
            CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);

            VisitData visitData;

            clang_visitChildren(cursor, visitTranslationUnit, &visitData);

            clang_disposeTranslationUnit(translationUnit);
         }

         for (CXString cxString : argumentsAsCXString)
         {
            clang_disposeString(cxString);
         }

         clang_disposeString(fileNameString);
      }

      clang_CompileCommands_dispose(compileCommands);
      clang_disposeIndex(index);
   }

   clang_CompilationDatabase_dispose(compilationDatabase);

   return 0;
}
