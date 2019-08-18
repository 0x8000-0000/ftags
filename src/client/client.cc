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

#include <ftags.pb.h>

#include <clara.hpp>

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
void dispatchFindAll(zmq::socket_t& socket, const std::string& symbolName)
{
   spdlog::info("Searching for symbol {}", symbolName);

   ftags::Command command{};
   command.set_source("client");
   std::string serializedCommand;

   command.set_type(ftags::Command::Type::Command_Type_QUERY);
   command.set_symbolname(symbolName);
   command.SerializeToString(&serializedCommand);

   zmq::message_t request(serializedCommand.size());
   memcpy(request.data(), serializedCommand.data(), serializedCommand.size());
   socket.send(request);

   //  Get the reply.
   zmq::message_t reply;
   socket.recv(&reply);

   ftags::Status status;
   status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));

   if (status.type() == ftags::Status_Type::Status_Type_QUERY_RESULTS)
   {
      zmq::message_t resultsMessage;
      socket.recv(&resultsMessage);

      ftags::BufferExtractor extractor(static_cast<std::byte*>(resultsMessage.data()), resultsMessage.size());
      const ftags::CursorSet output = ftags::CursorSet::deserialize(extractor);
      spdlog::info("Received {} results", output.size());

      for (auto iter = output.begin(); iter != output.end(); ++iter)
      {
         const ftags::Cursor cursor = output.inflateRecord(*iter);

         std::cout << cursor.location.fileName << ':' << cursor.location.line << ':' << cursor.location.column << "  "
                   << cursor.attributes.getRecordFlavor() << ' ' << cursor.attributes.getRecordType() << " >> "
                   << cursor.symbolName << std::endl;
      }
   }
}

void dispatchDumpTranslationUnit(zmq::socket_t& socket, const std::string& fileName)
{
   std::filesystem::path filePath{fileName};
   std::string           canonicalFilePathAsString{fileName};
   if (std::filesystem::exists(filePath))
   {
      std::filesystem::path canonicalFilePath = std::filesystem::canonical(filePath);
      canonicalFilePathAsString               = canonicalFilePath.string();
   }
   else
   {
      std::filesystem::path otherPath = std::filesystem::current_path() / filePath;
      if (std::filesystem::exists(otherPath))
      {
         std::filesystem::path canonicalFilePath = std::filesystem::canonical(otherPath);
         canonicalFilePathAsString               = canonicalFilePath.string();
      }
   }

   spdlog::info("Dumping translation unit {}", canonicalFilePathAsString);

   ftags::Command command{};
   command.set_source("client");
   std::string   serializedCommand;
   ftags::Status status;

   command.set_type(ftags::Command::Type::Command_Type_DUMP_TRANSLATION_UNIT);
   command.set_filename(canonicalFilePathAsString);
   command.SerializeToString(&serializedCommand);

   zmq::message_t request(serializedCommand.size());
   memcpy(request.data(), serializedCommand.data(), serializedCommand.size());
   socket.send(request);

   //  Get the reply.
   zmq::message_t reply;
   socket.recv(&reply);

   status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));

   if (status.type() == ftags::Status_Type::Status_Type_QUERY_RESULTS)
   {
      zmq::message_t resultsMessage;
      socket.recv(&resultsMessage);

      ftags::BufferExtractor extractor(static_cast<std::byte*>(resultsMessage.data()), resultsMessage.size());
      const ftags::CursorSet output = ftags::CursorSet::deserialize(extractor);
      spdlog::info("Received {} results", output.size());

      for (auto iter = output.begin(); iter != output.end(); ++iter)
      {
         const ftags::Cursor cursor = output.inflateRecord(*iter);

         std::cout << cursor.location.line << ':' << cursor.location.column << "  "
                   << cursor.attributes.getRecordFlavor() << ' ' << cursor.attributes.getRecordType() << " >> "
                   << cursor.symbolName << std::endl;
      }
   }
}

bool showHelp = false;

bool        findAll             = false;
bool        findFunction        = false;
bool        dumpTranslationUnit = false;
bool        doQuit              = false;
bool        doPing              = false;
std::string symbolName;
std::string fileName;

auto cli = clara::Help(showHelp) | clara::Opt(doQuit)["-q"]["--quit"]("Shutdown server") |
           clara::Opt(doPing)["-i"]["--ping"]("Ping server") |
           clara::Opt(findAll)["-a"]["--all"]("Find all occurrences of symbol") |
           clara::Opt(findFunction)["-f"]["--function"]("Find function") |
           clara::Opt(dumpTranslationUnit)["--dump"]("Dump symbols for translation unit") |
           clara::Opt(symbolName, "symbol")["-s"]["--symbol"]("Symbol name") |
           clara::Opt(fileName, "file")["--file"]("File name");

} // namespace

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

   const char*       xdgRuntimeDir  = std::getenv("XDG_RUNTIME_DIR");
   const std::string socketLocation = fmt::format("ipc://{}/ftags_server", xdgRuntimeDir);

   //  Prepare our context and socket
   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_REQ);

   spdlog::info("Connecting to ftags server...");
   socket.connect(socketLocation);

   ftags::Command command{};
   command.set_source("client");
   std::string   serializedCommand;
   ftags::Status status;

   if (doPing)
   {
      command.set_type(ftags::Command::Type::Command_Type_PING);
      command.SerializeToString(&serializedCommand);

      zmq::message_t request(serializedCommand.size());
      memcpy(request.data(), serializedCommand.data(), serializedCommand.size());
      socket.send(request);

      //  Get the reply.
      zmq::message_t reply;
      socket.recv(&reply);

      status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));

      spdlog::info("Received timestamp {} with status {}.", status.timestamp(), status.type());
   }

   if (findAll)
   {
      dispatchFindAll(socket, symbolName);
   }
   else if (dumpTranslationUnit)
   {
      dispatchDumpTranslationUnit(socket, fileName);
   }

   if (doQuit)
   {
      command.set_type(ftags::Command::Type::Command_Type_SHUT_DOWN);
      std::string quitCommand;
      command.SerializeToString(&quitCommand);

      zmq::message_t request(quitCommand.size());
      memcpy(request.data(), quitCommand.data(), quitCommand.size());
      spdlog::info("Sending Quit");
      socket.send(request);
   }

   google::protobuf::ShutdownProtobufLibrary();

   return 0;
}
