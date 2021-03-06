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

#include <services.h>

#include <zmq.hpp>

#include <spdlog/spdlog.h>

#include <signal.h>

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
   setupSignals();

   spdlog::info("Logger started");

   zmq::context_t context{1};
   zmq::socket_t  receiver{context, ZMQ_PULL};

   const char*       xdgRuntimeDir  = std::getenv("XDG_RUNTIME_DIR");
   const std::string socketLocation = fmt::format("ipc://{}/ftags_logger", xdgRuntimeDir);

   receiver.bind(socketLocation);

   spdlog::info("Connection established");

   while (true)
   {
      try
      {
         zmq::message_t sourceMsg;
         receiver.recv(&sourceMsg);

         zmq::message_t pidMsg;
         receiver.recv(&pidMsg);

         zmq::message_t levelMsg;
         receiver.recv(&levelMsg);

         zmq::message_t messageMsg;
         receiver.recv(&messageMsg);

         std::string source{static_cast<char*>(sourceMsg.data()), sourceMsg.size()};

         pid_t pid{};
         memcpy(&pid, pidMsg.data(), sizeof(pid_t));

         uint32_t levelUint32 = 0;
         assert(levelMsg.size() == sizeof(levelUint32));
         memcpy(&levelUint32, levelMsg.data(), sizeof(levelUint32));
         spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(levelUint32);

         std::string msg{static_cast<char*>(messageMsg.data()), messageMsg.size()};

         spdlog::log(level, "[{}-{}] {}", source, pid, msg);
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

   spdlog::info("Logger shutting down");

   return 0;
}
