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

#include <algorithm>
#include <map>
#include <random>
#include <vector>

#include <cstdint>

static void BM_STL_ImplementationLinear(benchmark::State& state)
{
   const uint32_t valueCount{static_cast<uint32_t>(state.range(0))};
   const uint32_t bucketCount{static_cast<uint32_t>(state.range(1))};

   for (auto _ : state)
   {
      std::map<uint32_t, std::vector<uint32_t>> indexMap;

      for (uint32_t kk = 0; kk < valueCount; kk++)
      {
         for (uint32_t ii = 1; ii <= bucketCount; ii++)
         {
            indexMap[ii].push_back(ii + kk);
         }
      }
   }
}


static void BM_STL_ImplementationRandom(benchmark::State& state)
{
   const uint32_t valueCount{static_cast<uint32_t>(state.range(0))};

   std::vector<uint32_t>                   values(valueCount);
   std::uniform_int_distribution<uint32_t> distribution(1, 65535);
   std::default_random_engine              generator;
   generator.seed(42);
   std::generate(values.begin(), values.end(), [&distribution, &generator]() { return distribution(generator); });

   for (auto _ : state)
   {
      uint32_t                                  lastValue = 1;
      std::map<uint32_t, std::vector<uint32_t>> indexMap;

      for (uint32_t val : values)
      {
         indexMap[lastValue].push_back(val);
         lastValue = val;
      }
   }
}


static void BM_IndexMap_ImplementationLinear(benchmark::State& state)
{
   const uint32_t valueCount{static_cast<uint32_t>(state.range(0))};
   const uint32_t bucketCount{static_cast<uint32_t>(state.range(1))};

   for (auto _ : state)
   {
      ftags::IndexMap indexMap;

      for (uint32_t kk = 0; kk < valueCount; kk++)
      {
         for (uint32_t ii = 1; ii <= bucketCount; ii++)
         {
            indexMap.add(ii, ii + kk);
         }
      }
   }
}

BENCHMARK(BM_STL_ImplementationLinear)->Args({64, 512})->Args({128, 1024});
BENCHMARK(BM_IndexMap_ImplementationLinear)->Args({64, 512})->Args({128, 1024});

static void BM_IndexMap_ImplementationRandom(benchmark::State& state)
{
   const uint32_t valueCount{static_cast<uint32_t>(state.range(0))};

   std::vector<uint32_t>                   values(valueCount);
   std::uniform_int_distribution<uint32_t> distribution(1, 65535);
   std::default_random_engine              generator;
   generator.seed(42);
   std::generate(values.begin(), values.end(), [&distribution, &generator]() { return distribution(generator); });

   for (auto _ : state)
   {
      uint32_t        lastValue = 1;
      ftags::IndexMap indexMap;

      for (uint32_t val : values)
      {
         indexMap.add(lastValue, val);
         lastValue = val;
      }
   }
}

BENCHMARK(BM_STL_ImplementationRandom)->Arg(16384)->Arg(65536)->Arg(256 * 1024);
BENCHMARK(BM_IndexMap_ImplementationRandom)->Arg(16384)->Arg(65536)->Arg(256 * 1024);

BENCHMARK_MAIN();
