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
#include "ha3/sql/ops/tvf/builtin/RerankBase.h"
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "ha3/sql/common/Log.h" // IWYU pragma: keep
#include "ha3/sql/ops/tvf/TvfFunc.h"
#include "ha3/sql/ops/tvf/TvfFuncCreatorR.h"
#include "navi/builder/ResourceDefBuilder.h"
#include "table/ComboComparator.h"
#include "table/Table.h"

namespace isearch {
namespace sql {
class SqlTvfProfileInfo;
} // namespace sql
} // namespace isearch

namespace autil {
namespace mem_pool {
class Pool;
} // namespace mem_pool
} // namespace autil

namespace isearch {
namespace sql {
class SimpleRerankTvfFunc : public RerankBase {
public:
    SimpleRerankTvfFunc();
    ~SimpleRerankTvfFunc();
    SimpleRerankTvfFunc(const SimpleRerankTvfFunc &) = delete;
    SimpleRerankTvfFunc &operator=(const SimpleRerankTvfFunc &) = delete;
    bool init(TvfFuncInitContext &context) override;

protected:
void makeUp(std::vector<table::Row>& target,
            const std::vector<table::Row>& extra);

bool computeTable(const table::TablePtr &input) override;

private:
    std::vector<int32_t> _quotaNums;
    std::vector<std::string> _sortFields;
    std::vector<bool> _sortFlags;
    std::string _quotaNumStr;
    std::string _sortStr;

    bool _needMakeUp;
    bool _needSort;
};

typedef std::shared_ptr<SimpleRerankTvfFunc> SimpleRerankTvfFuncPtr;

class SimpleRerankTvfFuncCreator : public TvfFuncCreatorR {
public:
    SimpleRerankTvfFuncCreator();
    ~SimpleRerankTvfFuncCreator();

public:
    void def(navi::ResourceDefBuilder &builder) const override {
        builder.name("SIMPLE_RERANK_BY_QUOTA_TVF_FUNC");
    }

private:
    SimpleRerankTvfFuncCreator(const SimpleRerankTvfFuncCreator &) = delete;
    SimpleRerankTvfFuncCreator &operator=(const SimpleRerankTvfFuncCreator &) = delete;

private:
    TvfFunc *createFunction(const SqlTvfProfileInfo &info) override;
};

typedef std::shared_ptr<SimpleRerankTvfFuncCreator> SimpleRerankTvfFuncCreatorPtr;
} // namespace sql
} // namespace isearch
