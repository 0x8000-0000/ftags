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

#include <ftags.pb.h>

#include <project.h>
#include <zmq_logger_sink.h>

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <cstdlib>
#include <ctime>

namespace
{
std::string getTimeStamp()
{
   auto now       = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);

   std::stringstream ss;
   ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
   return ss.str();
}

void dispatchFindAll(zmq::socket_t& socket, const ftags::ProjectDb& projectDb, const std::string& symbolName)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());

   spdlog::info("Received query for {}", symbolName);
   const std::vector<const ftags::Record*> queryResultsVector = projectDb.findSymbol(symbolName);
   spdlog::info("Found {} occurrences for {}", queryResultsVector.size(), symbolName);

   std::string serializedStatus;

   if (queryResultsVector.empty())
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_NO_RESULTS);
   }
   else
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_RESULTS);
   }

   const std::size_t headerSize = status.ByteSizeLong();
   zmq::message_t    reply(headerSize);
   status.SerializeToArray(reply.data(), static_cast<int>(headerSize));
   socket.send(reply, ZMQ_SNDMORE);

   const ftags::CursorSet queryResultsCursor = projectDb.inflateRecords(queryResultsVector);

   const std::size_t     payloadSize = queryResultsCursor.computeSerializedSize();
   zmq::message_t        resultsMessage(payloadSize);
   ftags::BufferInsertor insertor(static_cast<std::byte*>(resultsMessage.data()), payloadSize);
   queryResultsCursor.serialize(insertor);
   socket.send(resultsMessage);
}

void dispatchDumpTranslationUnit(zmq::socket_t&          socket,
                                 const ftags::ProjectDb& projectDb,
                                 const std::string&      fileName)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());

   spdlog::info("Received dump request for {}", fileName);
   const std::vector<const ftags::Record*> queryResultsVector = projectDb.dumpTranslationUnit(fileName);

   std::string serializedStatus;

   if (queryResultsVector.empty())
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_NO_RESULTS);
   }
   else
   {
      status.set_type(ftags::Status_Type::Status_Type_QUERY_RESULTS);
   }

   const std::size_t headerSize = status.ByteSizeLong();
   zmq::message_t    reply(headerSize);
   status.SerializeToArray(reply.data(), static_cast<int>(headerSize));
   socket.send(reply, ZMQ_SNDMORE);

   const ftags::CursorSet queryResultsCursor = projectDb.inflateRecords(queryResultsVector);

   const std::size_t     payloadSize = queryResultsCursor.computeSerializedSize();
   zmq::message_t        resultsMessage(payloadSize);
   ftags::BufferInsertor insertor(static_cast<std::byte*>(resultsMessage.data()), payloadSize);
   queryResultsCursor.serialize(insertor);
   socket.send(resultsMessage);
}

void dispatchUpdateTranslationUnit(zmq::socket_t& socket, ftags::ProjectDb& projectDb, const std::string& fileName)
{
   zmq::message_t payload;
   socket.recv(&payload);

   spdlog::info("Received {} bytes of serialized data for translation unit {}.", payload.size(), fileName);

   ftags::BufferExtractor extractor(static_cast<std::byte*>(payload.data()), payload.size());

   ftags::ProjectDb updatedTranslationUnit;
   ftags::ProjectDb::deserialize(extractor, updatedTranslationUnit);

   projectDb.updateFrom(fileName, updatedTranslationUnit);

   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_TRANSLATION_UNIT_UPDATED);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
   spdlog::info("Acknowledged translation unit {}.", fileName);
}

void dispatchPing(zmq::socket_t& socket)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_IDLE);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

void dispatchUnknownCommand(zmq::socket_t& socket)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_UNKNOWN);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}

void dispatchShutdown(zmq::socket_t& socket)
{
   ftags::Status status{};
   status.set_timestamp(getTimeStamp());
   status.set_type(ftags::Status_Type::Status_Type_SHUTTING_DOWN);

   const std::size_t replySize = status.ByteSizeLong();
   zmq::message_t    reply(replySize);
   status.SerializeToArray(reply.data(), static_cast<int>(replySize));

   socket.send(reply);
}
} // namespace

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   ftags::ProjectDb projectDb;

#if 0
   if (argc < 2)
   {
      spdlog::error("Compilation database argument missing");
      return -1;
   }
   ftags::parseProject(argv[1], projectDb);
#endif

   //  Prepare our context and socket
   zmq::context_t context(1);

   ftags::ZmqCentralLogger centralLogger{context, std::string{"server"}};

   spdlog::info("Started");

   const char*       xdgRuntimeDir  = std::getenv("XDG_RUNTIME_DIR");
   const std::string socketLocation = fmt::format("ipc://{}/ftags_server", xdgRuntimeDir);

   zmq::socket_t socket(context, ZMQ_REP);
   socket.bind(socketLocation);

   bool shuttingDown = false;

   while (!shuttingDown)
   {
      zmq::message_t request;
      ftags::Command command{};

      //  Wait for next request from client
      socket.recv(&request);
      command.ParseFromArray(request.data(), static_cast<int>(request.size()));
      spdlog::info("Received request from {}: {}", command.source(), command.Type_Name(command.type()));

      switch (command.type())
      {

      case ftags::Command_Type::Command_Type_QUERY:
         dispatchFindAll(socket, projectDb, command.symbolname());
         break;

      case ftags::Command_Type::Command_Type_DUMP_TRANSLATION_UNIT:
         dispatchDumpTranslationUnit(socket, projectDb, command.filename());
         break;

      case ftags::Command_Type::Command_Type_UPDATE_TRANSLATION_UNIT:
         dispatchUpdateTranslationUnit(socket, projectDb, command.filename());
         break;

      case ftags::Command_Type::Command_Type_PING:
         dispatchPing(socket);
         break;

      case ftags::Command_Type::Command_Type_SHUT_DOWN:
         dispatchShutdown(socket);
         shuttingDown = true;
         break;

      default:
         dispatchUnknownCommand(socket);
         break;
      }
   }

   spdlog::info("Shutting down");

   return 0;
}
