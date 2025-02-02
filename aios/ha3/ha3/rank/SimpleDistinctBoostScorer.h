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
#pragma once

#include <stdint.h>

#include "ha3/isearch.h"
#include "ha3/rank/DistinctBoostScorer.h"
#include "autil/Log.h" // IWYU pragma: keep

namespace isearch {
namespace rank {

class SimpleDistinctBoostScorer : public DistinctBoostScorer
{
public:
    SimpleDistinctBoostScorer(uint32_t distinctTimes, uint32_t distinctCount);
    ~SimpleDistinctBoostScorer();
public:
    score_t calculateBoost(uint32_t keyPosition, score_t rawScore);
private:
    uint32_t _distinctTimes;
    uint32_t _distinctCount;
private:
    AUTIL_LOG_DECLARE();
};

} // namespace rank
} // namespace isearch

