#include <ftags.pb.h>

#include <zmq.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <ctime>

std::string getTimeStamp()
{
   auto now       = std::chrono::system_clock::now();
   auto in_time_t = std::chrono::system_clock::to_time_t(now);

   std::stringstream ss;
   ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
   return ss.str();
}

int main()
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   //  Prepare our context and socket
   zmq::context_t context(1);
   zmq::socket_t  socket(context, ZMQ_REP);
   socket.bind("tcp://*:5555");

   while (true)
   {
      zmq::message_t request;
      ftags::Command command{};

      //  Wait for next request from client
      socket.recv(&request);
      command.ParseFromArray(request.data(), static_cast<int>(request.size()));
      std::cout << "Received request from " << command.source() << std::endl;

      //  Do some 'work'
      std::this_thread::sleep_for(std::chrono::seconds(1));

      //  Send reply back to client
      ftags::Status status{};
      status.set_timestamp(getTimeStamp());
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
      socket.send(reply);

      if (command.type() == ftags::Command_Type::Command_Type_SHUT_DOWN)
      {
         break;
      }
   }

   return 0;
}