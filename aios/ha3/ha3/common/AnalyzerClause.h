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

#include <memory>
#include <string>

#include "autil/Log.h" // IWYU pragma: keep
#include "ha3/common/ClauseBase.h"

namespace autil {
class DataBuffer;
}  // namespace autil

namespace isearch {
namespace common {

// do not need serialize
class AnalyzerClause : public ClauseBase
{
public:
    AnalyzerClause();
    ~AnalyzerClause();
private:
    AnalyzerClause(const AnalyzerClause &);
    AnalyzerClause& operator=(const AnalyzerClause &);
public:
    void serialize(autil::DataBuffer &dataBuffer) const override {
        return;
    }
    void deserialize(autil::DataBuffer &dataBuffer) override {
        return;
    }
    std::string toString() const override {
        return _originalString;
    }

private:
    AUTIL_LOG_DECLARE();
};

typedef std::shared_ptr<AnalyzerClause> AnalyzerClausePtr;

} // namespace common
} // namespace isearch

