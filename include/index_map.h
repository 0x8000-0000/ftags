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

#include <store.h>

#include <map>
#include <vector>

#include <cstdint>

namespace ftags
{

/** Maps a non-zero unsigned value to a bag of unsigned values
 *
 * Conceptually, consider this an optimized implementation of
 *    std::map<uint32_t, std::vector<uint32_t>>
 */
class IndexMap
{
public:

   using const_iterator = typename Store<uint32_t, uint32_t, 24>::const_iterator;

   void add(uint32_t key, uint32_t value);

   /** Returns a range via begin and end iterators.
    */
   std::pair<const_iterator, const_iterator> getValues(uint32_t key) const noexcept;

   void removeKey(uint32_t key);

   void removeValue(uint32_t key, uint32_t value);

private:

   // the initial capacity of a bag
   static constexpr unsigned InitialAllocationSize = 6;

   // when growing, the new size is (X + X / GrowthFactor)
   static constexpr unsigned GrowthFactor = 4;

   /* we use two extra elements, one to store the key copy and one to
    * store the capacity, size pair
    */
   static constexpr unsigned MetadataSize = 2;

   /*
    * Allocate contiguous blocks, and use the following format:
    *    * key
    *    * block-size / block-capacity packed into 32 bits
    *    * element1, element2...
    *
    * If a bag is full, we either re-allocate it, or we reallocate
    * the next bag and we take over its space.
    */

   Store<uint32_t, uint32_t, 24> m_store;

   /* Maps from a value to the location in store where its corresponding bag
    * is stored.
    */
   std::map<uint32_t, uint32_t> m_index;
};

} // namespace ftags

#endif // INDEX_MAP_H_INCLUDED
