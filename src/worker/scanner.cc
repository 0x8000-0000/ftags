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

#include <zmq_logger_sink.h>

#include <ftags.pb.h>
#include <services.h>

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   const std::string loggerConnectionString = std::string("tcp://*:") + std::to_string(ftags::LoggerPort);
   auto sink = std::make_shared<ftags::ZmqLoggerSinkSinglethreaded>(std::string{"scanner"}, loggerConnectionString);
   spdlog::default_logger()->sinks().clear();
   spdlog::default_logger()->sinks().push_back(sink);
   spdlog::default_logger()->set_pattern("%v");

   spdlog::info("Indexer started");

   if (argc < 2)
   {
      return 0;
   }

   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_PUSH);

   const std::string connectionString = std::string("tcp://*:") + std::to_string(ftags::WorkerPort);
   socket.bind(connectionString);

   spdlog::info("Connection established");

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

         CXString dirNameString  = clang_CompileCommand_getDirectory(compileCommand);
         CXString fileNameString = clang_CompileCommand_getFilename(compileCommand);

         const unsigned        argCount = clang_CompileCommand_getNumArgs(compileCommand);
         std::vector<CXString> argumentsAsCXString;
         argumentsAsCXString.reserve(argCount);

         ftags::IndexRequest indexRequest{};
         indexRequest.set_projectname("main");
         indexRequest.set_directoryname(clang_getCString(dirNameString));
         indexRequest.set_filename(clang_getCString(fileNameString));

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

            indexRequest.add_argument(argumentText);
         }

         std::string serializedRequest;
         indexRequest.SerializeToString(&serializedRequest);

         zmq::message_t request(serializedRequest.size());
         memcpy(request.data(), serializedRequest.data(), serializedRequest.size());
         socket.send(request);

         spdlog::info("Enqueued {}", clang_getCString(fileNameString));

         for (CXString cxString : argumentsAsCXString)
         {
            clang_disposeString(cxString);
         }

         clang_disposeString(fileNameString);
         clang_disposeString(dirNameString);
      }

      clang_CompileCommands_dispose(compileCommands);
      clang_disposeIndex(index);
   }

   clang_CompilationDatabase_dispose(compilationDatabase);

   return 0;
}
