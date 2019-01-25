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

#include <index_map.h>

#include <algorithm>
#include <random>

#include <cassert>

// const uint32_t loopCount = 42;      // 41 is ok
// const uint32_t bucketCount = 3;

// const uint32_t loopCount = 11;      // 10 is ok
// const uint32_t bucketCount = 4;

// const uint32_t loopCount = 23;      // 22 is ok
// const uint32_t bucketCount = 5;

const uint32_t loopCount   = 1024;
const uint32_t bucketCount = 16384;

int main(void)
{
   ftags::IndexMap indexMap;

#if 0
   for (uint32_t kk = 0; kk < loopCount; kk++)
   {
      for (uint32_t ii = 1; ii <= bucketCount; ii++)
      {
         auto range = indexMap.getValues(ii);

         uint32_t vv = 0;
         for (auto iter = range.first; iter != range.second; ++ iter)
         {
            assert(*iter == (ii * 100 + vv));
            vv++;
         }

         indexMap.add(ii, ii * 100 + kk);

         indexMap.validateInternalState();
      }
   }
#else

   std::vector<uint32_t>                   values(1024 * 1024);
   std::uniform_int_distribution<uint32_t> distribution(1, 65535);
   std::default_random_engine              generator;
   generator.seed(42);
   std::generate(values.begin(), values.end(), [&distribution, &generator]() { return distribution(generator); });

   uint32_t lastValue = 1;

   for (uint32_t val : values)
   {
      indexMap.add(lastValue, val);
      lastValue = val;
   }
#endif

   return 0;
}
