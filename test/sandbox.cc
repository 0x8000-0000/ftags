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
#include <iostream>
#include <random>

#define TEST_LINEAR

// const uint32_t loopCount = 42;      // 41 is ok
// const uint32_t bucketCount = 3;

// const uint32_t loopCount = 11;      // 10 is ok
// const uint32_t bucketCount = 4;

// const uint32_t loopCount = 23;      // 22 is ok
// const uint32_t bucketCount = 5;

#ifdef TEST_LINEAR
const uint32_t loopCount   = 1024;
const uint32_t bucketCount = 16384;
#endif

static void testLinear()
{
   std::cout << "Test linear..." << std::endl;

   ftags::IndexMap indexMap;

   for (uint32_t kk = 0; kk < loopCount; kk++)
   {
      for (uint32_t ii = 1; ii <= bucketCount; ii++)
      {
         auto range = indexMap.getValues(ii);

         uint32_t vv = 0;
         for (auto iter = range->first; iter != range->second; ++ iter)
         {
            //assert(*iter == (ii * 100 + vv));
            if (*iter != (ii * 100 + vv))
            {
               throw std::logic_error("No match");
            }
            vv++;
         }

         indexMap.add(ii, ii * 100 + kk);

         indexMap.validateInternalState();
      }
   }

   std::cout << "Test linear completed" << std::endl;
}

static void testRandom()
{
   std::cout << "Test random..." << std::endl;

   ftags::IndexMap indexMap;
   std::map<uint32_t, std::vector<uint32_t>> stlIndexMap;

   std::vector<uint32_t>                   values(1024 * 1024);
   std::uniform_int_distribution<uint32_t> distribution(1, 65535);
   std::default_random_engine              generator;
   generator.seed(42);
   std::generate(values.begin(), values.end(), [&distribution, &generator]() { return distribution(generator); });

   uint32_t lastValue = 1;

   for (uint32_t val : values)
   {
      indexMap.add(lastValue, val);
      stlIndexMap[lastValue].push_back(val);
      lastValue = val;
   }

   for (const auto& pp: stlIndexMap)
   {
      auto range{indexMap.getValues(pp.first)};
      if (! range)
      {
         throw std::logic_error("Missing values");
      }

      if (! std::equal(range->first, range->second, pp.second.begin()))
      {
         throw std::logic_error("Values mismatch");
      }
   }

   std::cout << "Test random completed" << std::endl;
}

int main(void)
{
   testLinear();

   testRandom();

   return 0;
}
