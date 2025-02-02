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
#include <memory>
#include <string>

#include "autil/Log.h" // IWYU pragma: keep
#include "ha3/search/BatchAggregateSampler.h"
#include "ha3/search/MultiAggregator.h"

namespace autil {
namespace mem_pool {
class Pool;
}  // namespace mem_pool
}  // namespace autil
namespace matchdoc {
class MatchDoc;
}  // namespace matchdoc

namespace isearch {
namespace search {

class BatchMultiAggregator : public MultiAggregator
{
public:
    BatchMultiAggregator(autil::mem_pool::Pool *pool);
    BatchMultiAggregator(const AggregatorVector &aggVec, autil::mem_pool::Pool *pool);
    virtual ~BatchMultiAggregator();
private:
    BatchMultiAggregator(const BatchMultiAggregator &);
    BatchMultiAggregator& operator=(const BatchMultiAggregator &);
public:
    void doAggregate(matchdoc::MatchDoc matchDoc) override;
    void estimateResult(double factor) override;
    void endLayer(double factor) override;
    std::string getName() override {
        return "batchMultiAgg";
    }
    uint32_t getAggregateCount() override {
        return _sampler.getAggregateCount();
    }
private:
    BatchAggregateSampler _sampler;
private:
    AUTIL_LOG_DECLARE();
};

typedef std::shared_ptr<BatchMultiAggregator> BatchMultiAggregatorPtr;

} // namespace search
} // namespace isearch

