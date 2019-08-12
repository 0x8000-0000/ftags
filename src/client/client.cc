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

#include <clara.hpp>

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <string>

bool        findFunction = false;
bool        doQuit       = false;
bool        doPing       = false;
std::string symbolName;

auto cli = clara::Opt(doQuit)["-q"]["--quit"]("Shutdown server") | clara::Opt(doPing)["-i"]["--ping"]("Ping server") |
           clara::Opt(findFunction)["-f"]["--function"]("Find function") |
           clara::Opt(symbolName, "symbol")["-s"]["--symbol"]("Symbol name");

int main(int argc, char* argv[])
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   auto result = cli.parse(clara::Args(argc, argv));
   if (!result)
   {
      spdlog::error("Failed to parse command line options: {}", result.errorMessage());
      exit(-1);
   }

   //  Prepare our context and socket
   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_REQ);

   spdlog::info("Connecting to ftags server...");
   socket.connect("tcp://localhost:5555");

   ftags::Command command{};
   command.set_source("client");
   std::string serializedCommand;
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

   if (findFunction)
   {
      spdlog::info("Searching for function {}", symbolName);

      command.set_type(ftags::Command::Type::Command_Type_QUERY);
      command.set_symbol(symbolName);
      command.SerializeToString(&serializedCommand);

      zmq::message_t request(serializedCommand.size());
      memcpy(request.data(), serializedCommand.data(), serializedCommand.size());
      socket.send(request);

      //  Get the reply.
      zmq::message_t reply;
      socket.recv(&reply);

      status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));
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
