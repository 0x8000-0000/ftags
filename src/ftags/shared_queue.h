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

#ifndef SHARED_QUEUE_H_INCLUDED
#define SHARED_QUEUE_H_INCLUDED

#include <condition_variable>
#include <mutex>
#include <queue>

namespace ftags
{

template <typename T>
class shared_queue
{
   using value_type = T;

public:
   void push(const value_type& value)
   {
      std::lock_guard<std::mutex> lock{m_mutex};

      m_data.push(value);
      m_dataNotEmpty.notify_one();
   }

   void push(value_type&& value)
   {
      std::lock_guard<std::mutex> lock{m_mutex};

      m_data.push(value);
      m_dataNotEmpty.notify_one();
   }

   value_type pop()
   {
      std::unique_lock<std::mutex> lock{m_mutex};
      m_dataNotEmpty.wait(lock, [this] { return !m_data.empty(); });

      value_type value(std::move(m_data.front()));
      m_data.pop();
      return value;
   }

private:
   std::queue<T>           m_data;
   std::mutex              m_mutex;
   std::condition_variable m_dataNotEmpty;
};

} // namespace ftags

#endif // SHARED_QUEUE_H_INCLUDED
