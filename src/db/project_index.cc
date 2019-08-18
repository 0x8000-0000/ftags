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

#ifndef NDEBUG
#include <fstream>
#endif
#include <string>
#include <thread>
#include <vector>

namespace
{
const unsigned k_minThreadCount = 6;

struct CompilationRequest
{
   std::string              directoryName;
   std::string              fileName;
   std::vector<std::string> arguments;

   bool done = false;
};

const char* g_defaultArguments[] = {
   "-isystem",
   "/usr/include",
   "-isystem",
   "/usr/include/c++/8",
   "-isystem",
   "/usr/include/x86_64-linux-gnu/c++/8",
   "-isystem",
   "/usr/lib/gcc/x86_64-linux-gnu/8/include",
};

void dumpTranslationUnit(const ftags::TranslationUnit& translationUnit, const std::string& fileName)
{
#if 0
   std::ofstream out(fileName);

   std::vector<const ftags::Record*> records = translationUnit.getRecords(true);
   for (const ftags::Record* record : records)
   {
      out << record->startLine << ':' << record->startColumn << "  " << record->attributes.getRecordFlavor() << ' '
          << record->attributes.getRecordType() << " >> " << record->symbolNameKey << std::endl;
   }
#else
   (void)translationUnit;
   (void)fileName;
#endif
}

void parseTranslationUnit(ftags::ProjectDb&                        projectDb,
                          ftags::shared_queue<CompilationRequest>& compilationRequests,
                          std::mutex&                              clangMutex)
{
   while (true)
   {
      CompilationRequest request = compilationRequests.pop();
      if (request.done)
      {
         break;
      }

      ftags::StringTable symbolTable;
      ftags::StringTable fileNameTable;

      std::vector<const char*> arguments;
      arguments.reserve(request.arguments.size() + std::size(g_defaultArguments));

#if 0
      /*
       * these were needed at some point
       */
      for (size_t ii = 0; ii < std::size(g_defaultArguments); ii++)
      {
         arguments.push_back(g_defaultArguments[ii]);
      }
#endif

      for (const auto& arg : request.arguments)
      {
         arguments.push_back(arg.data());
      }

      spdlog::debug("Parsing {}", request.fileName);

      try
      {
         clangMutex.lock();

         ftags::TranslationUnit translationUnit =
            ftags::TranslationUnit::parse(request.fileName, arguments, symbolTable, fileNameTable);

         clangMutex.unlock();

         spdlog::info("Loaded {} records from {}, {} from main file",
                      translationUnit.getRecordCount(),
                      request.fileName,
                      translationUnit.getRecords(true).size());

         const ftags::TranslationUnit& mergedTranslationUnit =
            projectDb.addTranslationUnit(request.fileName, translationUnit, symbolTable, fileNameTable);

         dumpTranslationUnit(translationUnit, request.fileName + ".orig");
         dumpTranslationUnit(mergedTranslationUnit, request.fileName + ".merged");
      }
      catch (const std::runtime_error& re)
      {
         spdlog::error("Failed to parse {}: {}", re.what());
      }
   }
}
} // anonymous namespace

void ftags::parseProject(const char* parentDirectory, ftags::ProjectDb& projectDb)
{
   ftags::shared_queue<CompilationRequest> compilationRequests;

   std::mutex clangMutex;

   CXCompilationDatabase_Error ccderror      = CXCompilationDatabase_NoError;
   CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory(parentDirectory, &ccderror);

   std::vector<std::thread> parserThreads;
   const auto               hardwareConcurrency = std::thread::hardware_concurrency();
   const unsigned           threadCount         = (hardwareConcurrency == 0) ? k_minThreadCount : hardwareConcurrency;
   parserThreads.reserve(/* size = */ threadCount);

   spdlog::info("Using {} threads", threadCount);

   for (unsigned ii = 0; ii < threadCount; ii++)
   {
      parserThreads.emplace_back(
         parseTranslationUnit, std::ref(projectDb), std::ref(compilationRequests), std::ref(clangMutex));
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

         compilationRequests.push(std::move(compilationRequest));

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
      compilationRequests.push(doneSentinel);
   }

   for (unsigned ii = 0; ii < threadCount; ii++)
   {
      parserThreads[ii].join();
   }

   spdlog::info("Using {} bytes for the database.", projectDb.computeSerializedSize());
   spdlog::info("Database contains {} records.", projectDb.getRecordCount());

   spdlog::info("All threads completed");

   clang_CompilationDatabase_dispose(compilationDatabase);
}

void ftags::parseOneFile(const std::string& fileName, std::vector<const char*> arguments, ftags::ProjectDb& projectDb)
{
   ftags::StringTable symbolTable;
   ftags::StringTable fileNameTable;

   spdlog::debug("Parsing {}", fileName);

   try
   {
      ftags::TranslationUnit translationUnit =
         ftags::TranslationUnit::parse(fileName, arguments, symbolTable, fileNameTable);

      spdlog::info("Loaded {:n} records from {}, {:n} from main file",
                   translationUnit.getRecordCount(),
                   fileName,
                   translationUnit.getRecords(true).size());

      projectDb.addTranslationUnit(fileName, translationUnit, symbolTable, fileNameTable);
   }
   catch (const std::runtime_error& re)
   {
      spdlog::error("Failed to parse {}: {}", re.what());
   }
}
