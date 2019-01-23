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

#include <benchmark/benchmark.h>

#include <map>
#include <vector>

#include <cstdint>

static void BM_STL_Implementation(benchmark::State& state)
{
   const uint32_t bucketCount{static_cast<uint32_t>(state.range(0))};
   
   std::map<uint32_t, std::vector<uint32_t>> indexMap;

   for (auto _ : state)
   {
      for (uint32_t kk = 0; kk < 16; kk++)
      {
         for (uint32_t ii = 1; ii <= bucketCount; ii++)
         {
            indexMap[ii].push_back(ii + kk);
         }
      }
   }
}

BENCHMARK(BM_STL_Implementation)->Arg(128)->Arg(256);


static void BM_IndexMap_Implementation(benchmark::State& state)
{
   const uint32_t bucketCount{static_cast<uint32_t>(state.range(0))};
   
   ftags::IndexMap indexMap;

   for (auto _ : state)
   {
      for (uint32_t kk = 0; kk < 16; kk++)
      {
         for (uint32_t ii = 1; ii <= bucketCount; ii++)
         {
            indexMap.add(ii, ii + kk);
         }
      }
   }
}

BENCHMARK(BM_IndexMap_Implementation)->Arg(128)->Arg(256);

BENCHMARK_MAIN();
