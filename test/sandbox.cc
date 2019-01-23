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

const uint32_t bucketCount = 128;

int main(void)
{
   ftags::IndexMap indexMap;

   for (uint32_t kk = 0; kk < 1024; kk++)
   {
      for (uint32_t ii = 1; ii <= bucketCount; ii++)
      {
         indexMap.add(ii, ii + kk);
      }
   }

   return 0;
}
