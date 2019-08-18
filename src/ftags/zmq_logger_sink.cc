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

#include "zmq_logger_sink.h"

#include <spdlog/spdlog.h>

#include <unistd.h>

ftags::ZmqPublisher::ZmqPublisher(zmq::context_t& context, const std::string& name) :
   m_name{name},
   m_socket{context, ZMQ_PUSH}
{
   const char*       xdgRuntimeDir  = std::getenv("XDG_RUNTIME_DIR");
   const std::string socketLocation = fmt::format("ipc://{}/ftags_logger", xdgRuntimeDir);

   m_socket.connect(socketLocation);

   m_pid = getpid();
}

void ftags::ZmqPublisher::publish(spdlog::level::level_enum level, const std::string& msg)
{
   zmq::message_t sourceMsg{m_name.size()};
   memcpy(sourceMsg.data(), m_name.data(), m_name.size());
   m_socket.send(sourceMsg, ZMQ_SNDMORE);

   zmq::message_t pidMsg{sizeof(pid_t)};
   memcpy(pidMsg.data(), &m_pid, sizeof(pid_t));
   m_socket.send(pidMsg, ZMQ_SNDMORE);

   uint32_t       levelUint32{level};
   zmq::message_t levelMsg{sizeof(levelUint32)};
   memcpy(levelMsg.data(), &levelUint32, sizeof(levelUint32));
   m_socket.send(levelMsg, ZMQ_SNDMORE);

   zmq::message_t messageMsg{msg.size() - 1};
   memcpy(messageMsg.data(), msg.data(), msg.size() - 1);
   m_socket.send(messageMsg, 0);
}

ftags::ZmqCentralLogger::ZmqCentralLogger(zmq::context_t& context, const std::string& name)
{
   auto sink = std::make_shared<ftags::ZmqLoggerSinkSinglethreaded>(context, name);

   // TODO: save default logger and restore it in the destructor

   spdlog::default_logger()->sinks().clear();
   spdlog::default_logger()->sinks().push_back(sink);
   spdlog::default_logger()->set_pattern("%v");
}

ftags::ZmqCentralLogger::~ZmqCentralLogger()
{
   spdlog::default_logger()->sinks().clear();
}
