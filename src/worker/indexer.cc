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

#include <signal.h>

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

static volatile int s_interrupted = 0;

static void signalHandler(int /* signal_value */)
{
   s_interrupted = 1;
}

static void setupSignals(void)
{
   struct sigaction action
   {
   };
   action.sa_handler = signalHandler;
   action.sa_flags   = 0;
   sigemptyset(&action.sa_mask);
   sigaction(SIGINT, &action, NULL);
   sigaction(SIGTERM, &action, NULL);
}

int main()
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   setupSignals();

   zmq::context_t context(1);

   ftags::ZmqCentralLogger centralLogger{context, std::string{"indexer"}};

   spdlog::info("Indexer started");

   zmq::socket_t receiver(context, ZMQ_PULL);

   const char*       xdgRuntimeDir    = std::getenv("XDG_RUNTIME_DIR");
   const std::string connectionString = fmt::format("ipc://{}/ftags_worker", xdgRuntimeDir);
   receiver.connect(connectionString);

   const std::string serverLocation = fmt::format("ipc://{}/ftags_server", xdgRuntimeDir);
   zmq::socket_t     serverSocket(context, ZMQ_REQ);
   serverSocket.connect(serverLocation);

   spdlog::info("Connection established");

   bool shutdownRequested{false};

   while (!shutdownRequested)
   {
      try
      {
         zmq::message_t message;
         spdlog::info("Waiting");
         receiver.recv(&message);

         ftags::IndexRequest indexRequest{};
         indexRequest.ParseFromArray(message.data(), static_cast<int>(message.size()));
         shutdownRequested = indexRequest.shutdownafter();

         spdlog::info("Received index request with {} translation units", indexRequest.translationunit_size());

         ftags::ProjectDb projectDb{/* name = */ indexRequest.projectname(),
                                    /* rootDirectory = */ indexRequest.directoryname()};

         for (int tt = 0; tt < indexRequest.translationunit_size(); tt++)
         {
            ftags::TranslationUnitArguments translationUnitArguments = indexRequest.translationunit(tt);

            spdlog::info("Processing {}", translationUnitArguments.filename());

            std::vector<const char*> arguments;
            const int                argCount = translationUnitArguments.argument_size();
            arguments.reserve(static_cast<size_t>(argCount));
            for (int ii{0}; ii < argCount; ++ii)
            {
               arguments.push_back(translationUnitArguments.argument(ii).c_str());
            }

            projectDb.parseOneFile(translationUnitArguments.filename(), arguments, indexRequest.indexeverything());

            projectDb.assertValid();
         }

         ftags::Command command{};
         command.set_source("indexer");
         command.set_type(ftags::Command::Type::Command_Type_UPDATE_TRANSLATION_UNIT);
         command.set_projectname(projectDb.getName());
         command.set_directoryname(projectDb.getRoot());

         for (int tt = 0; tt < indexRequest.translationunit_size(); tt++)
         {
            command.add_translationunit(indexRequest.translationunit(tt).filename());
         }

         const std::size_t headerSize = command.ByteSizeLong();
         zmq::message_t    header(headerSize);
         command.SerializeToArray(header.data(), static_cast<int>(headerSize));
         serverSocket.send(header, ZMQ_SNDMORE);

         const std::size_t           payloadSize = projectDb.computeSerializedSize();
         zmq::message_t              projectMessage(payloadSize);
         ftags::util::BufferInsertor insertor(static_cast<std::byte*>(projectMessage.data()), payloadSize);
         projectDb.serialize(insertor);

         serverSocket.send(projectMessage);

         /*
          * wait for the server to acknowledge
          */
         zmq::message_t reply;
         serverSocket.recv(&reply);
         ftags::Status status;
         status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));
      }
      catch (zmq::error_t& ze)
      {
         spdlog::error("0mq exception: {}", ze.what());
      }
      if (s_interrupted)
      {
         spdlog::debug("interrupt received; stopping worker");
         break;
      }
   }

   spdlog::info("Indexer shutting down");

   return 0;
}
