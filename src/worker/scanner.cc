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

#include <clara.hpp>

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace
{
bool        showHelp = false;
std::string projectName;
std::string dirName;
int         groupSize = 5;

auto cli = clara::Help(showHelp) |
           clara::Opt(groupSize, "group")["--group"]("How many translation units to parse at once") |
           clara::Opt(projectName, "project")["-p"]["--project"]("Project name") |
           clara::Arg(dirName, "dir")("Path to directory containing compile_commands.json");

} // anonymous namespace

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   auto result = cli.parse(clara::Args(argc, argv));
   if (!result)
   {
      spdlog::error("Failed to parse command line options: {}", result.errorMessage());
      exit(-1);
   }

   if (showHelp)
   {
      std::cout << cli << std::endl;
      exit(0);
   }

   std::filesystem::path projectPath{dirName};
   {
      std::filesystem::path compilationDatabaseFile = projectPath / "compile_commands.json";
      if (std::filesystem::exists(compilationDatabaseFile))
      {
         std::filesystem::path canonicalProjectPath = std::filesystem::canonical(projectPath);
         dirName                                    = canonicalProjectPath.string();
         spdlog::info(fmt::format("Scanning project {} in {}", projectName, dirName));
      }
      else
      {
         std::cout << fmt::format("Specified directory {} does not contain a compilation database file.", dirName)
                   << std::endl;
         exit(-2);
      }
   }

   zmq::context_t context(1);

   ftags::ZmqCentralLogger centralLogger{context, std::string{"scanner"}};

   spdlog::info("Started");

   if (argc < 2)
   {
      return 0;
   }

   zmq::socket_t socket(context, ZMQ_PUSH);

   const char*       xdgRuntimeDir    = std::getenv("XDG_RUNTIME_DIR");
   const std::string connectionString = fmt::format("ipc://{}/ftags_worker", xdgRuntimeDir);

   socket.bind(connectionString);

   /*
    * Allow all clients time to bind.
    *
    * See https://github.com/zeromq/pyzmq/issues/96
    */
   using namespace std::chrono_literals;
   std::this_thread::sleep_for(1s);

   CXCompilationDatabase_Error ccderror      = CXCompilationDatabase_NoError;
   CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory(dirName.c_str(), &ccderror);

   if (CXCompilationDatabase_NoError == ccderror)
   {
      CXIndex index = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                        /* displayDiagnostics         = */ 0);

      CXCompileCommands compileCommands = clang_CompilationDatabase_getAllCompileCommands(compilationDatabase);

      const unsigned compilationCount = clang_CompileCommands_getSize(compileCommands);

      ftags::IndexRequest indexRequest{};
      indexRequest.set_projectname(projectName);
      indexRequest.set_directoryname(dirName);

      for (unsigned ii = 0; ii < compilationCount; ii++)
      {
         CXCompileCommand compileCommand = clang_CompileCommands_getCommand(compileCommands, ii);

         CXString dirNameString  = clang_CompileCommand_getDirectory(compileCommand);
         CXString fileNameString = clang_CompileCommand_getFilename(compileCommand);

         const unsigned        argCount = clang_CompileCommand_getNumArgs(compileCommand);
         std::vector<CXString> argumentsAsCXString;
         argumentsAsCXString.reserve(argCount);

         ftags::TranslationUnitArguments* translationUnit = indexRequest.add_translationunit();

         translationUnit->set_filename(clang_getCString(fileNameString));

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
            }
            else
            {
               translationUnit->add_argument(argumentText);
            }

            clang_disposeString(cxString);
         }

         if (indexRequest.translationunit_size() == groupSize)
         {
            const std::size_t requestSize = indexRequest.ByteSizeLong();
            zmq::message_t    request(requestSize);
            indexRequest.SerializeToArray(request.data(), static_cast<int>(requestSize));
            socket.send(request);

            spdlog::info("Enqueued {} of {}: {}", ii, compilationCount, clang_getCString(fileNameString));

            indexRequest.clear_translationunit();
         }

         clang_disposeString(fileNameString);
         clang_disposeString(dirNameString);
      }

      if (indexRequest.translationunit_size())
      {
         std::string serializedRequest;
         indexRequest.SerializeToString(&serializedRequest);

         zmq::message_t request(serializedRequest.size());
         memcpy(request.data(), serializedRequest.data(), serializedRequest.size());
         socket.send(request);

         spdlog::info("Enqueued last batch of {} translation units", indexRequest.translationunit_size());

         indexRequest.clear_translationunit();
      }

      spdlog::info("Done with enqueueing");

      clang_CompileCommands_dispose(compileCommands);
      clang_disposeIndex(index);
   }

   clang_CompilationDatabase_dispose(compilationDatabase);

   spdlog::info("Shutting down");

   socket.close();

   return 0;
}
