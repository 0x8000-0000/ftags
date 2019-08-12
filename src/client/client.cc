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

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <string>

int main()
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   //  Prepare our context and socket
   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_REQ);

   std::cout << "Connecting to hello world server…" << std::endl;
   socket.connect("tcp://localhost:5555");

   ftags::Command command{};
   command.set_source("client");
   command.set_type(ftags::Command::Type::Command_Type_PING);
   std::string serializedCommand;
   command.SerializeToString(&serializedCommand);

   ftags::Status status;

   // Send 3 requests, waiting each time for a response
   for (int request_nbr = 0; request_nbr != 3; request_nbr++)
   {
      zmq::message_t request(serializedCommand.size());
      memcpy(request.data(), serializedCommand.data(), serializedCommand.size());
      std::cout << "Sending Ping #" << request_nbr << "" << std::endl;
      socket.send(request);

      //  Get the reply.
      zmq::message_t reply;
      socket.recv(&reply);
      
      status.ParseFromArray(reply.data(), static_cast<int>(reply.size()));
      
      std::cout << "Received timestamp " << status.timestamp() << " with status " << status.type() << std::endl;
   }

   {
      command.set_type(ftags::Command::Type::Command_Type_SHUT_DOWN);
      std::string quitCommand;
      command.SerializeToString(&quitCommand);

      zmq::message_t request(quitCommand.size());
      memcpy(request.data(), quitCommand.data(), quitCommand.size());
      std::cout << "Sending Quit" << std::endl;
      socket.send(request);
   }

   google::protobuf::ShutdownProtobufLibrary();

   return 0;
}
