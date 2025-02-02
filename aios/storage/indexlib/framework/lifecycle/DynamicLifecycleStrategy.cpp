/*
 * Copyright 2014-present Alibaba Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "indexlib/framework/lifecycle/DynamicLifecycleStrategy.h"

#include "indexlib/framework/SegmentDescriptions.h"
#include "indexlib/framework/SegmentStatistics.h"
#include "indexlib/framework/lifecycle/LifecycleTableCreator.h"

using namespace std;

namespace indexlib::framework {

std::vector<std::pair<segmentid_t, std::string>> DynamicLifecycleStrategy::GetSegmentLifecycles(
    const shared_ptr<indexlibv2::framework::SegmentDescriptions>& segDescriptions)
{
    std::vector<std::pair<segmentid_t, std::string>> ret;
    const auto& segStatistics = segDescriptions->GetSegmentStatisticsVector();
    for (const auto& segStats : segStatistics) {
        auto temperature = CalculateLifecycle(segStats);
        if (!temperature.empty()) {
            ret.emplace_back(segStats.GetSegmentId(), temperature);
        }
    }
    return ret;
}

string
DynamicLifecycleStrategy::CalculateLifecycle(const indexlibv2::framework::SegmentStatistics& segmentStatistic) const
{
    return indexlibv2::framework::LifecycleTableCreator::CalculateLifecycle(segmentStatistic, _lifecycleConfig);
}

} // namespace indexlib::framework
