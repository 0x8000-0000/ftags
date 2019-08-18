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
} // namespace

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   if (argc < 2)
   {
      spdlog::error("Compilation database argument missing");
      return -1;
   }

   const char* xdgRuntimeDir = std::getenv("XDG_RUNTIME_DIR");

   const std::string socketLocation = fmt::format("ipc://{}/ftags_server", xdgRuntimeDir);

   ftags::ProjectDb projectDb;

   ftags::parseProject(argv[1], projectDb);

   //  Prepare our context and socket
   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_REP);
   socket.bind(socketLocation);

   while (true)
   {
      zmq::message_t request;
      ftags::Command command{};

      //  Wait for next request from client
      socket.recv(&request);
      command.ParseFromArray(request.data(), static_cast<int>(request.size()));
      spdlog::info("Received request from {}: {}", command.source(), command.Type_Name(command.type()));

      if (command.type() == ftags::Command_Type::Command_Type_QUERY)
      {
         dispatchFindAll(socket, projectDb, command.symbolname());

         continue;
      }

      if (command.type() == ftags::Command_Type::Command_Type_DUMP_TRANSLATION_UNIT)
      {
         dispatchDumpTranslationUnit(socket, projectDb, command.filename());

         continue;
      }

      ftags::Status status{};
      status.set_timestamp(getTimeStamp());

      //  Send reply back to client
      if (command.type() == ftags::Command_Type::Command_Type_SHUT_DOWN)
      {
         status.set_type(ftags::Status_Type::Status_Type_SHUTTING_DOWN);
      }
      else
      {
         status.set_type(ftags::Status_Type::Status_Type_IDLE);
      }

      std::string serializedStatus;
      status.SerializeToString(&serializedStatus);
      zmq::message_t reply(serializedStatus.size());
      memcpy(reply.data(), serializedStatus.data(), serializedStatus.size());
      socket.send(reply, ZMQ_SNDMORE);

      if (command.type() == ftags::Command_Type::Command_Type_SHUT_DOWN)
      {
         break;
      }
   }

   return 0;
}
