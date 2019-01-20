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

#ifndef INDEX_MAP_H_INCLUDED
#define INDEX_MAP_H_INCLUDED

#include <map>
#include <vector>

#include <cstdint>

namespace ftags
{

/** Maps a non-zero unsigned value to a bag of unsigned values
 */
class IndexMap
{
public:
   IndexMap();

   void add(uint32_t key, uint32_t value);

   std::vector<uint32_t> getValues(uint32_t key);

   void removeKey(uint32_t key);

   void removeValue(uint32_t key, uint32_t value);

private:

   /*
    * Maintain the bags in a compact form:
    *    - a set of buckets
    *    - inside each bucket, the bags are adjacent
    *       - uint16_t capacity
    *       - uint16_t size / used
    *       - uint32_t[size] values
    *       - uint32_t[capacity - size] uninitialized values
    *       - if capacity == size == (1<<16 - 1) then the bag continues with
    *         the next set of elements
    *    - if a bag has exausted its capacity, it is re-allocated with a larger size, the data copied and the old
    *      bag available for reuse
    */

   std::vector<std::vector<uint32_t>> m_buckets;

   std::map<uint32_t, uint32_t> m_registry;
};

} // namespace ftags

#endif // INDEX_MAP_H_INCLUDED
