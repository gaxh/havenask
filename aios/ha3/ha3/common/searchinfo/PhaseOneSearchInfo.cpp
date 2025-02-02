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
#include "ha3/common/searchinfo/PhaseOneSearchInfo.h"

#include <cstddef>

#include "autil/DataBuffer.h"
#include "autil/StringUtil.h"
#include "autil/TimeUtility.h"
#include "ha3/common/searchinfo/QrsSearchInfo.h"

using namespace std;
using namespace autil;
namespace isearch {
namespace common {

PhaseOneSearchInfo::PhaseOneSearchInfo(uint32_t partitionCount_,
                                       uint32_t useTruncateOptimizerCount_,
                                       uint32_t fromFullCacheCount_,
                                       uint32_t seekCount_,
                                       uint32_t matchCount_,
                                       int64_t rankLatency_,
                                       int64_t rerankLatency_,
                                       int64_t extraLatency_,
                                       int64_t searcherProcessLatency_,
                                       uint32_t seekDocCount_)
    : partitionCount(partitionCount_)
    , useTruncateOptimizerCount(useTruncateOptimizerCount_)
    , fromFullCacheCount(fromFullCacheCount_)
    , seekCount(seekCount_)
    , seekDocCount(seekDocCount_)
    , matchCount(matchCount_)
    , rankLatency(rankLatency_)
    , rerankLatency(rerankLatency_)
    , extraLatency(extraLatency_)
    , searcherProcessLatency(searcherProcessLatency_)
{
    needReport = true;
}

PhaseOneSearchInfo::~PhaseOneSearchInfo() {
}

void PhaseOneSearchInfo::serialize(autil::DataBuffer &dataBuffer) const {
    dataBuffer.write(partitionCount);
    dataBuffer.write(useTruncateOptimizerCount);
    dataBuffer.write(fromFullCacheCount);
    dataBuffer.write(seekCount);
    dataBuffer.write(matchCount);
    dataBuffer.write(rankLatency);
    dataBuffer.write(rerankLatency);
    dataBuffer.write(extraLatency);
    dataBuffer.write(searcherProcessLatency);
    dataBuffer.write(otherInfoStr);
    dataBuffer.write(qrsSearchInfo);
}

void PhaseOneSearchInfo::deserialize(autil::DataBuffer &dataBuffer) {
    dataBuffer.read(partitionCount);
    dataBuffer.read(useTruncateOptimizerCount);
    dataBuffer.read(fromFullCacheCount);
    dataBuffer.read(seekCount);
    dataBuffer.read(matchCount);
    dataBuffer.read(rankLatency);
    dataBuffer.read(rerankLatency);
    dataBuffer.read(extraLatency);
    dataBuffer.read(searcherProcessLatency);
    dataBuffer.read(otherInfoStr);
    dataBuffer.read(qrsSearchInfo);
}

void PhaseOneSearchInfo::mergeFrom(
        const vector<PhaseOneSearchInfo*> &searchInfos)
{
    for (size_t i = 0; i < searchInfos.size(); ++i) {
        partitionCount += searchInfos[i]->partitionCount;
        useTruncateOptimizerCount += searchInfos[i]->useTruncateOptimizerCount;
        fromFullCacheCount += searchInfos[i]->fromFullCacheCount;
        seekCount += searchInfos[i]->seekCount;
        seekDocCount += searchInfos[i]->seekDocCount;
        matchCount += searchInfos[i]->matchCount;
        rankLatency += searchInfos[i]->rankLatency;
        rerankLatency += searchInfos[i]->rerankLatency;
        extraLatency += searchInfos[i]->extraLatency;
        searcherProcessLatency += searchInfos[i]->searcherProcessLatency;
        otherInfoStr += searchInfos[i]->otherInfoStr;
    }
}

string PhaseOneSearchInfo::toString() const {
    string result;
    result += StringUtil::toString(partitionCount) + "_";
    result += StringUtil::toString(useTruncateOptimizerCount) + "_";
    result += StringUtil::toString(fromFullCacheCount) + "_";
    result += StringUtil::toString(seekCount) + "_";
    result += StringUtil::toString(matchCount) + "_";
    result += StringUtil::toString(seekDocCount) + "_";
    result += StringUtil::toString(rankLatency) + "us_";
    result += StringUtil::toString(rerankLatency) + "us_";
    result += StringUtil::toString(extraLatency) + "us_";
    result += StringUtil::toString(searcherProcessLatency) + "us_";
    result += "o:" + otherInfoStr + "_";
    result +=  "q:" +qrsSearchInfo.getQrsSearchInfo();
    return result;
}

void PhaseOneSearchInfo::reset() {
    partitionCount = 0;
    useTruncateOptimizerCount = 0;
    fromFullCacheCount = 0;
    seekCount = 0;
    seekDocCount = 0;
    matchCount = 0;
    rankLatency = 0;
    rerankLatency = 0;
    extraLatency = 0;
    searcherProcessLatency = 0;
    needReport = true;
    otherInfoStr.clear();
    qrsSearchInfo.reset();
}

} // namespace common
} // namespace isearch
