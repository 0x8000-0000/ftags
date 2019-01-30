#include <ftags.pb.h>

#include <zmq.hpp>

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
