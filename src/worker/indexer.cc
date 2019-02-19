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

#include <iostream>
#include <string>
#include <vector>

#if 0
namespace
{

struct VisitData
{
   unsigned int level = 0;
};

void dumpCursorInfo(CXCursor cursor, unsigned int level)
{
   CXString name = clang_getCursorSpelling(cursor);

   CXCursorKind kind     = clang_getCursorKind(cursor);
   CXString     kindName = clang_getCursorKindSpelling(kind);

   CXSourceLocation location = clang_getCursorLocation(cursor);

   CXString     fileName;
   unsigned int line   = 0;
   unsigned int column = 0;

   clang_getPresumedLocation(location, &fileName, &line, &column);

   std::cout << clang_getCString(fileName) << ':' << line << ':' << column << ':' << std::string(level, ' ')
             << clang_getCString(kindName) << ':' << clang_getCString(name) << std::endl;

   clang_disposeString(fileName);
   clang_disposeString(kindName);
   clang_disposeString(name);
}

CXChildVisitResult visitTranslationUnit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
   VisitData* visitData = reinterpret_cast<VisitData*>(clientData);

   dumpCursorInfo(cursor, visitData->level);

   VisitData childrenData;
   childrenData.level = visitData->level + 1;
   clang_visitChildren(cursor, visitTranslationUnit, &childrenData);

   return CXChildVisit_Continue;
}

} // anonymous namespace
#endif

int main()
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   zmq::context_t context(1);

   ftags::ZmqCentralLogger centralLogger{context, std::string{"indexer"}, ftags::LoggerPort};

   spdlog::info("Indexer started");

   zmq::socket_t  receiver(context, ZMQ_PULL);

   const std::string connectionString = std::string("tcp://localhost:") + std::to_string(ftags::WorkerPort);
   receiver.connect(connectionString);

   spdlog::info("Connection established");

   bool shutdownRequested{false};

   while (!shutdownRequested)
   {
      zmq::message_t message;
      receiver.recv(&message);

      ftags::IndexRequest indexRequest{};
      indexRequest.ParseFromArray(message.data(), static_cast<int>(message.size()));
      shutdownRequested = indexRequest.shutdownafter();

      spdlog::info("Received message");

      CXIndex index = clang_createIndex(/* excludeDeclarationsFromPCH = */ 0,
                                        /* displayDiagnostics         = */ 0);

      std::vector<const char*> arguments;
      const int                argCount = indexRequest.argument_size();
      arguments.reserve(static_cast<size_t>(argCount));
      for (int ii{0}; ii < argCount; ++ii)
      {
         arguments.push_back(indexRequest.argument(ii).c_str());
      }

      CXTranslationUnit translationUnit = nullptr;

      spdlog::info("Processing {}", indexRequest.filename());

      const CXErrorCode parseError =
         clang_parseTranslationUnit2(/* CIdx                  = */ index,
                                     /* source_filename       = */ indexRequest.filename().c_str(),
                                     /* command_line_args     = */ arguments.data(),
                                     /* num_command_line_args = */ static_cast<int>(arguments.size()),
                                     /* unsaved_files         = */ nullptr,
                                     /* num_unsaved_files     = */ 0,
                                     /* options               = */ CXTranslationUnit_DetailedPreprocessingRecord |
                                        CXTranslationUnit_SingleFileParse,
                                     /* out_TU                = */ &translationUnit);

      spdlog::info("Parse status for {}: {}", indexRequest.filename(), parseError);
      if (parseError == CXError_Success)
      {
#if 0
         CXCursor cursor = clang_getTranslationUnitCursor(translationUnit);
         VisitData visitData;
         clang_visitChildren(cursor, visitTranslationUnit, &visitData);
#endif

         clang_disposeTranslationUnit(translationUnit);
      }

      clang_disposeIndex(index);
   }

   return 0;
}
