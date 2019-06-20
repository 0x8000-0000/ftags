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

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <spdlog/spdlog.h>

#include <shared_queue.h>

#include <string>
#include <thread>
#include <vector>

const unsigned k_minThreadCount = 6;

struct CompilationRequest
{
   std::string              directoryName;
   std::string              fileName;
   std::vector<std::string> arguments;

   bool done = false;
};

ftags::ProjectDb g_projectDb;

ftags::shared_queue<CompilationRequest> g_compilationRequests;

void parseTranslationUnit()
{
   while (true)
   {
      CompilationRequest request = g_compilationRequests.pop();
      if (request.done)
      {
         break;
      }

      ftags::StringTable symbolTable;
      ftags::StringTable fileNameTable;

      std::vector<const char*> arguments;
      arguments.reserve(request.arguments.size());

      for (const auto& arg : request.arguments)
      {
         arguments.push_back(arg.data());
      }

      spdlog::debug("Parsing {}", request.fileName);

      ftags::TranslationUnit translationUnit =
         ftags::TranslationUnit::parse(request.fileName, arguments, symbolTable, fileNameTable);

      spdlog::info("Loaded {} records from {}", translationUnit.getRecordCount(), request.fileName);

      g_projectDb.addTranslationUnit(request.fileName, translationUnit);
   }
}

int main(int /* argc */, char* argv[])
{
   CXCompilationDatabase_Error ccderror            = CXCompilationDatabase_NoError;
   CXCompilationDatabase       compilationDatabase = clang_CompilationDatabase_fromDirectory(argv[1], &ccderror);

   std::vector<std::thread> parserThreads;
   const auto               hardwareConcurrency = std::thread::hardware_concurrency();
   const unsigned           threadCount         = (hardwareConcurrency == 0) ? k_minThreadCount : hardwareConcurrency;
   parserThreads.reserve(/* size = */ threadCount);

   spdlog::info("Using {} threads", threadCount);

   for (unsigned ii = 0; ii < threadCount; ii++)
   {
      parserThreads.emplace_back(parseTranslationUnit);
   }

   if (CXCompilationDatabase_NoError == ccderror)
   {
      spdlog::info("Start enqueueing");

      CXIndex index = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                        /* displayDiagnostics         = */ 0);

      CXCompileCommands compileCommands = clang_CompilationDatabase_getAllCompileCommands(compilationDatabase);

      const unsigned compilationCount = clang_CompileCommands_getSize(compileCommands);

      for (unsigned ii = 0; ii < compilationCount; ii++)
      {
         CXCompileCommand compileCommand = clang_CompileCommands_getCommand(compileCommands, ii);

         CXString dirNameString  = clang_CompileCommand_getDirectory(compileCommand);
         CXString fileNameString = clang_CompileCommand_getFilename(compileCommand);

         const unsigned argCount = clang_CompileCommand_getNumArgs(compileCommand);

         CompilationRequest compilationRequest;
         compilationRequest.directoryName = clang_getCString(dirNameString);
         compilationRequest.fileName      = clang_getCString(fileNameString);

         bool skipFileNames = false;

         compilationRequest.arguments.reserve(argCount);

         for (unsigned jj = 0; jj < argCount; jj++)
         {
            if (skipFileNames)
            {
               skipFileNames = false;
               continue;
            }

            CXString    cxString     = clang_CompileCommand_getArg(compileCommand, jj);
            const char* argumentText = clang_getCString(cxString);

            if ((argumentText[0] == '-') && ((argumentText[1] == 'c') || (argumentText[1] == 'o')))
            {
               skipFileNames = true;
            }
            else
            {
               compilationRequest.arguments.emplace_back(argumentText);
            }
            clang_disposeString(cxString);
         }

         g_compilationRequests.push(std::move(compilationRequest));

         clang_disposeString(fileNameString);
         clang_disposeString(dirNameString);
      }

      spdlog::info("Done with enqueueing");

      clang_CompileCommands_dispose(compileCommands);
      clang_disposeIndex(index);
   }

   spdlog::info("Sending done signal");

   for (unsigned ii = 0; ii < threadCount; ii++)
   {
      CompilationRequest doneSentinel;
      doneSentinel.done = true;
      g_compilationRequests.push(doneSentinel);
   }

   for (unsigned ii = 0; ii < threadCount; ii++)
   {
      parserThreads[ii].join();
   }

   spdlog::info("All threads completed");

   clang_CompilationDatabase_dispose(compilationDatabase);
   return 0;
}
