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

#ifndef FTAGS_STATISTICS_H_INCLUDED
#define FTAGS_STATISTICS_H_INCLUDED

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cassert>
#include <cmath>

namespace ftags::stats
{

template <typename T>
struct FiveNumbersSummary
{
   T minimum;
   T lowerQuartile;
   T median;
   T upperQuartile;
   T maximum;
};

template <typename T>
class Sample
{
public:
   void addValue(T val)
   {
      m_valueCount[val]++;

      m_sampleCount++;
   }

   std::vector<std::string> prepareHistogram(unsigned bucketCount);

   FiveNumbersSummary<T> computeFiveNumberSummary() const;

   unsigned getSampleCount() const
   {
      return m_sampleCount;
   }

private:
   std::map<T, unsigned> m_valueCount;

   unsigned m_sampleCount = 0;
};

/*
 * implementation
 */
template <typename T>
FiveNumbersSummary<T> Sample<T>::computeFiveNumberSummary() const
{
   FiveNumbersSummary<T> summary = {};

   const std::size_t valuesCount = m_valueCount.size();

   std::vector<T> values;
   values.reserve(valuesCount);
   std::vector<unsigned> cumulativeCounts;
   cumulativeCounts.reserve(valuesCount);

   unsigned totalSampleCount = 0;

   /*
    * take advantage that iterating over a map yields the values in sorted
    * order
    */
   for (const auto& [vv, cc] : m_valueCount)
   {
      values.push_back(vv);

      totalSampleCount += cc;

      cumulativeCounts.push_back(totalSampleCount);
   }

   const unsigned lowerQuartileCount = totalSampleCount / 4;
   const unsigned medianCount        = totalSampleCount / 2;
   const unsigned upperQuartileCount = totalSampleCount - lowerQuartileCount;

   const auto medianIter = std::lower_bound(cumulativeCounts.cbegin(), cumulativeCounts.cend(), medianCount);
   const auto medianPos  = std::distance(cumulativeCounts.cbegin(), medianIter);
   assert(medianPos >= 0);

   const auto lowerQuartileIter = std::lower_bound(cumulativeCounts.cbegin(), medianIter, lowerQuartileCount);
   const auto lowerQuartilePos  = std::distance(cumulativeCounts.cbegin(), lowerQuartileIter);
   assert(lowerQuartilePos >= 0);

   const auto upperQuartileIter = std::lower_bound(medianIter, cumulativeCounts.cend(), upperQuartileCount);
   const auto upperQuartilePos  = std::distance(cumulativeCounts.cbegin(), upperQuartileIter);
   assert(upperQuartilePos >= 0);

   summary.minimum       = values[0];
   summary.lowerQuartile = values[static_cast<std::size_t>(lowerQuartilePos)];
   summary.median        = values[static_cast<std::size_t>(medianPos)];
   summary.upperQuartile = values[static_cast<std::size_t>(upperQuartilePos)];
   summary.maximum       = values[valuesCount - 1];

   return summary;
}

template <typename T>
std::vector<std::string> Sample<T>::prepareHistogram(unsigned bucketCount)
{
   FiveNumbersSummary<T> summary = computeFiveNumberSummary();

   if (bucketCount == 0)
   {
      /*
       * https://en.wikipedia.org/wiki/Freedman%E2%80%93Diaconis_rule
       */
      const T      iqr      = summary.upperQuartile - summary.lowerQuartile;
      const double binWidth = (2 * iqr) / std::pow(m_sampleCount, 1 / 3.0);
      bucketCount           = (summary.maximum - summary.minimum) / binWidth;
   }

   std::vector<std::string> display(/* size = */ bucketCount);

   return display;
}

} // namespace ftags::stats

#endif // FTAGS_STATISTICS_H_INCLUDED
