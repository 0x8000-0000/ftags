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

#include <query.h>

#include <ftags.pb.h>

#include <fmt/format.h>

#include <clara.hpp>

#include <zmq.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace
{

bool beVerbose = false;

void dispatchFindAll(zmq::socket_t&     socket,
                     const std::string& projectName,
                     const std::string& dirName,
                     const std::string& symbolName)
{
   if (beVerbose)
   {
      std::cout << fmt::format("Searching for symbol {}\n", symbolName);
   }

   ftags::Command command{};
   command.set_source("client");
   std::string serializedCommand;

   command.set_type(ftags::Command::Type::Command_Type_QUERY);
   command.set_projectname(projectName);
   command.set_directoryname(dirName);
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

      ftags::util::BufferExtractor extractor(static_cast<std::byte*>(resultsMessage.data()), resultsMessage.size());
      const ftags::CursorSet       output = ftags::CursorSet::deserialize(extractor);
      if (beVerbose)
      {
         std::cout << fmt::format("Received {} results\n", output.size());
      }

      for (auto iter = output.begin(); iter != output.end(); ++iter)
      {
         const ftags::Cursor cursor = output.inflateRecord(*iter);

         std::cout << cursor.location.fileName << ':' << cursor.location.line << ':' << cursor.location.column << "  "
                   << cursor.attributes.getRecordFlavor() << ' ' << cursor.attributes.getRecordType() << " >> "
                   << cursor.symbolName << std::endl;
      }
   }
   else if (status.type() == ftags::Status_Type::Status_Type_UNKNOWN_PROJECT)
   {
      std::cout << "Unknown project: '" << projectName << "'" << std::endl;

      for (int ii = 0; ii < status.remarks_size(); ii++)
      {
         std::cout << status.remarks(ii) << std::endl;
      }
   }
}

void dispatchIdentifySymbol(zmq::socket_t&     socket,
                            const std::string& projectName,
                            const std::string& dirName,
                            const std::string& fileName,
                            unsigned           lineNumber,
                            unsigned           columnNumber)
{
   if (beVerbose)
   {
      std::cout << fmt::format("Identifying symbol at {}:{}:{}\n", fileName, lineNumber, columnNumber);
   }

   ftags::Command command{};
   command.set_source("client");
   std::string serializedCommand;

   command.set_type(ftags::Command::Type::Command_Type_QUERY);
   command.set_querytype(ftags::Command_QueryType_IDENTIFY);
   command.set_projectname(projectName);
   command.set_directoryname(dirName);
   command.set_filename(fileName);
   command.set_linenumber(lineNumber);
   command.set_columnnumber(columnNumber);
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

      ftags::util::BufferExtractor extractor(static_cast<std::byte*>(resultsMessage.data()), resultsMessage.size());
      const ftags::CursorSet       output = ftags::CursorSet::deserialize(extractor);
      if (beVerbose)
      {
         std::cout << fmt::format("Received {} results\n", output.size());
      }

      for (auto iter = output.begin(); iter != output.end(); ++iter)
      {
         const ftags::Cursor cursor = output.inflateRecord(*iter);

         std::cout << cursor.location.fileName << ':' << cursor.location.line << ':' << cursor.location.column << "  "
                   << cursor.attributes.getRecordFlavor() << ' ' << cursor.attributes.getRecordType() << " >> "
                   << cursor.symbolName << std::endl;
      }
   }
   else if (status.type() == ftags::Status_Type::Status_Type_UNKNOWN_PROJECT)
   {
      std::cout << "Unknown project: '" << projectName << "'" << std::endl;

      for (int ii = 0; ii < status.remarks_size(); ii++)
      {
         std::cout << status.remarks(ii) << std::endl;
      }
   }
}

void dispatchDumpTranslationUnit(zmq::socket_t&     socket,
                                 const std::string& projectName,
                                 const std::string& dirName,
                                 const std::string& fileName)
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

   if (beVerbose)
   {
      std::cout << fmt::format("Dumping translation unit {}", canonicalFilePathAsString);
   }

   ftags::Command command{};
   command.set_source("client");
   std::string   serializedCommand;
   ftags::Status status;

   command.set_type(ftags::Command::Type::Command_Type_DUMP_TRANSLATION_UNIT);
   command.set_projectname(projectName);
   command.set_directoryname(dirName);
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

      ftags::util::BufferExtractor extractor(static_cast<std::byte*>(resultsMessage.data()), resultsMessage.size());
      const ftags::CursorSet       output = ftags::CursorSet::deserialize(extractor);
      if (beVerbose)
      {
         std::cout << fmt::format("Received {} results\n", output.size());
      }

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

bool                     findAll             = false;
bool                     findFunction        = false;
bool                     dumpTranslationUnit = false;
bool                     doQuit              = false;
bool                     doPing              = false;
bool                     queryStats          = false;
std::string              projectName;
std::string              symbolName;
std::string              fileName;
std::string              dirName;
std::vector<std::string> queryArray;

auto cli = clara::Help(showHelp) | clara::Opt(doQuit)["-q"]["--quit"]("Shutdown server") |
           clara::Opt(beVerbose)["-v"]["--verbose"]("Verbose stats") |
           clara::Opt(doPing)["-i"]["--ping"]("Ping server") |
           clara::Opt(queryStats)["-q"]["--stats"]("Query statistics from running server") |
           clara::Opt(projectName, "project")["-p"]["--project"]("Project name") |
           clara::Opt(dirName, "directory")["-d"]["--directory"]("Project root directory") |
           clara::Opt(findAll)["-a"]["--all"]("Find all occurrences of symbol") |
           clara::Opt(findFunction)["-f"]["--function"]("Find function") |
           clara::Opt(dumpTranslationUnit)["--dump"]("Dump symbols for translation unit") |
           clara::Opt(symbolName, "symbol")["-s"]["--symbol"]("Symbol name") |
           clara::Opt(fileName, "file")["--file"]("File name") | clara::Arg(queryArray, "query");

} // namespace

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   auto result = cli.parse(clara::Args(argc, argv));
   if (!result)
   {
      std::cout << fmt::format("Failed to parse command line options: {}\n", result.errorMessage());
      exit(-1);
   }

   // canonicalize the project directory input
   {
      if (dirName.empty())
      {
         dirName = ".";
      }

      std::filesystem::path projectPath{dirName};
      std::filesystem::path canonicalProjectPath = std::filesystem::canonical(projectPath);
      dirName                                    = canonicalProjectPath.string();
   }

   if (showHelp)
   {
      std::cout << cli << std::endl;
      exit(0);
   }

   ftags::query::Query query;

   try
   {
      query = ftags::query::Query::parse(queryArray);
   }
   catch (std::runtime_error& runtimeError)
   {
      std::cout << "Failed to parse query: " << runtimeError.what() << std::endl;
      exit(1);
   }

   const char*       xdgRuntimeDir  = std::getenv("XDG_RUNTIME_DIR");
   const std::string socketLocation = fmt::format("ipc://{}/ftags_server", xdgRuntimeDir);

   //  Prepare our context and socket
   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_REQ);

   if (beVerbose)
   {
      std::cout << "Connecting to ftags server..." << std::endl;
   }
   socket.connect(socketLocation);

   ftags::Command command{};
   command.set_source("client");
   std::string   serializedCommand;
   ftags::Status status;

   if (query.verb == ftags::query::Query::Verb::Ping)
   {
      command.set_type(ftags::Command::Type::Command_Type_PING);
      const std::size_t requestSize = command.ByteSizeLong();
      zmq::message_t    request(requestSize);
      command.SerializeToArray(request.data(), static_cast<int>(requestSize));
      socket.send(request);

      zmq::message_t reply;
      socket.recv(&reply);

      status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));

      if (beVerbose)
      {
         std::cout << fmt::format("Received timestamp {} with status {}\n", status.timestamp(), status.type());
      }
   }
   else if (query.verb == ftags::query::Query::Verb::Find)
   {
      dispatchFindAll(socket, projectName, dirName, query.symbolName);
   }
   else if (query.verb == ftags::query::Query::Verb::Identify)
   {
      dispatchIdentifySymbol(socket, projectName, dirName, query.filePath, query.lineNumber, query.columnNumber);
   }
   else if (query.verb == ftags::query::Query::Verb::Dump)
   {
      if (query.type == ftags::query::Query::Type::Statistics)
      {
         command.set_type(ftags::Command::Type::Command_Type_QUERY_STATISTICS);
         command.set_projectname(projectName);
         command.set_directoryname(dirName);
         command.set_symbolname(query.symbolName);
         const std::size_t requestSize = command.ByteSizeLong();
         zmq::message_t    request(requestSize);
         command.SerializeToArray(request.data(), static_cast<int>(requestSize));
         socket.send(request);

         zmq::message_t reply;
         socket.recv(&reply);

         status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));

         for (int ii = 0; ii < status.remarks_size(); ii++)
         {
            std::cout << status.remarks(ii) << std::endl;
         }
      }
      else if (query.type == ftags::query::Query::Type::Contents)
      {
         dispatchDumpTranslationUnit(socket, projectName, dirName, fileName);
      }
   }
   else if (query.verb == ftags::query::Query::Verb::Shutdown)
   {
      command.set_type(ftags::Command::Type::Command_Type_SHUT_DOWN);
      std::string quitCommand;
      command.SerializeToString(&quitCommand);

      zmq::message_t request(quitCommand.size());
      memcpy(request.data(), quitCommand.data(), quitCommand.size());
      if (beVerbose)
      {
         std::cout << "Sending Quit\n";
      }
      socket.send(request);
   }

   google::protobuf::ShutdownProtobufLibrary();

   return 0;
}
