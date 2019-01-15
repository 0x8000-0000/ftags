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
#include <vector>

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      return 1;
   }

   CXCompilationDatabase_Error ccderror = CXCompilationDatabase_NoError;

   CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory(argv[1], &ccderror);

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

         const unsigned           argCount = clang_CompileCommand_getNumArgs(compileCommand);
         std::vector<CXString>    argumentsAsCXString(argCount);
         std::vector<const char*> arguments(argCount);

         for (unsigned jj = 0; jj < argCount; jj++)
         {
            argumentsAsCXString[jj] = clang_CompileCommand_getArg(compileCommand, jj);
            arguments[jj]           = clang_getCString(argumentsAsCXString[jj]);
            std::cout << "   " << arguments[jj] << std::endl;
         }

         CXTranslationUnit translationUnit = nullptr;

         const CXErrorCode parseError =
            clang_parseTranslationUnit2(/* CIdx                  = */ index,
                                        /* source_filename       = */ sourceFileName,
                                        /* command_line_args     = */ arguments.data(),
                                        /* num_command_line_args = */ static_cast<int>(argCount),
                                        /* unsaved_files         = */ nullptr,
                                        /* num_unsaved_files     = */ 0,
                                        /* options               = */ 0,
                                        /* out_TU                = */ &translationUnit);

         std::cout << "   Parse status: " << parseError << std::endl;
         if (parseError == CXError_Success)
         {
            clang_disposeTranslationUnit(translationUnit);
         }

         for (unsigned jj = 0; jj < argCount; jj++)
         {
            clang_disposeString(argumentsAsCXString[jj]);
         }

         clang_disposeString(fileNameString);
      }

      clang_CompileCommands_dispose(compileCommands);
      clang_disposeIndex(index);
   }

   clang_CompilationDatabase_dispose(compilationDatabase);

   return 0;
}
