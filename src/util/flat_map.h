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

#ifndef FTAGS_FLAT_MAP_H_INCLUDED
#define FTAGS_FLAT_MAP_H_INCLUDED

#include <algorithm>
#include <utility>
#include <vector>

namespace ftags
{

template <typename K, typename V>
class FlatMap
{
public:

   using const_iterator = typename std::vector<std::pair<K, V>>::const_iterator;

   FlatMap(std::vector<std::pair<K, V>>&& data) : m_data{data}
   {
      std::sort(m_data.begin(), m_data.end(), [](const std::pair<K, V>& left, const std::pair<K, V>& right) -> bool {
         return left.first < right.first;
      });
   }

   const_iterator lookup(K kk) const
   {
      const_iterator iter = std::lower_bound(m_data.cbegin(), m_data.cend(), kk, [](const std::pair<K, V>& left, K val) -> bool {
         return left.first < val;
      });

      if (iter->first == kk)
      {
         return iter;
      }
      else
      {
         return m_data.cend();
      }
   }

   const_iterator none() const
   {
      return m_data.cend();
   }

private:
   std::vector<std::pair<K, V>> m_data;
};

template <typename K, typename V>
class FlatMapAccumulator
{
public:
   FlatMapAccumulator(std::size_t size)
   {
      m_data.reserve(size);
   }

   void add(K kk, V vv)
   {
      m_data.emplace_back(kk, vv);
   }

   FlatMap<K, V> getMap()
   {
      return FlatMap<K, V>(std::move(m_data));
   }

private:
   std::vector<std::pair<K, V>> m_data;
};

} // namespace ftags

#endif // FTAGS_FLAT_MAP_H_INCLUDED
