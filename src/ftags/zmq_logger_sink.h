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

#ifndef ZMQ_LOGGER_SINK_H_INCLUDED
#define ZMQ_LOGGER_SINK_H_INCLUDED

#include <zmq.hpp>

#include <spdlog/sinks/base_sink.h>

#include <sys/types.h>

namespace ftags
{

class ZmqPublisher
{
public:
   ZmqPublisher(zmq::context_t& context, const std::string& name, const std::string& connectionString);

   void publish(spdlog::level::level_enum level, const std::string& msg);

private:
   std::string m_name;

   zmq::socket_t m_socket;

   pid_t m_pid;
};

class ZmqCentralLogger
{
   public:
      ZmqCentralLogger(zmq::context_t& context, const std::string& name, int loggerPort);
      ~ZmqCentralLogger();
};

template <typename Mutex>
class ZmqLoggerSink : public spdlog::sinks::base_sink<Mutex>
{
public:
   ZmqLoggerSink(zmq::context_t& context, const std::string& name, const std::string& connectionString) :
      m_publisher{context, name, connectionString}
   {
   }

protected:
   void sink_it_(const spdlog::details::log_msg& msg) override
   {
      fmt::memory_buffer formatted;
      spdlog::sinks::sink::formatter_->format(msg, formatted);
      m_publisher.publish(msg.level, fmt::to_string(formatted));
   }

   void flush_() override
   {
      // nothing to do
   }

private:
   ZmqPublisher m_publisher;
};

#include <mutex>
#include <spdlog/details/null_mutex.h>

using ZmqLoggerSinkMultithreaded  = ZmqLoggerSink<std::mutex>;
using ZmqLoggerSinkSinglethreaded = ZmqLoggerSink<spdlog::details::null_mutex>;

} // namespace ftags

#endif // ZMQ_LOGGER_SINK_H_INCLUDED
