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
#include <stdint.h>
#include <string>
#include <vector>

#include "autil/Log.h"
#include "autil/legacy/jsonizable.h"
#include "ha3/sql/common/common.h"
#include "ha3/sql/ops/tvf/TvfWrapper.h"
#include "navi/common.h"
#include "navi/engine/Kernel.h"
#include "navi/engine/KernelConfigContext.h"
#include "table/Table.h"

namespace autil {
namespace mem_pool {
class Pool;
} // namespace mem_pool
} // namespace autil
namespace isearch {
namespace sql {
class SqlBizResource;
class SqlQueryResource;
class MetaInfoResource;
class SqlSearchInfoCollector;
} // namespace sql
} // namespace isearch
namespace navi {
class GraphMemoryPoolResource;
class KernelComputeContext;
class KernelDefBuilder;
class KernelInitContext;
} // namespace navi

namespace isearch {
namespace sql {

class TvfKernel : public navi::Kernel {
public:
    TvfKernel();
    ~TvfKernel();

public:
    void def(navi::KernelDefBuilder &builder) const override;
    bool config(navi::KernelConfigContext &ctx) override;
    navi::ErrorCode init(navi::KernelInitContext &initContext) override;
    navi::ErrorCode compute(navi::KernelComputeContext &runContext) override;

private:
    TvfInitParam _initParam;
    TvfWrapperPtr _tvfWrapper;
    SqlBizResource *_bizResource = nullptr;
    SqlQueryResource *_queryResource = nullptr;
    navi::GraphMemoryPoolResource *_memoryPoolResource = nullptr;
    MetaInfoResource *_metaInfoResource = nullptr;

private:
    AUTIL_LOG_DECLARE();
};

typedef std::shared_ptr<TvfKernel> TvfKernelPtr;
} // namespace sql
} // namespace isearch
