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

#include "navi/engine/Resource.h"
#include "ha3/common/QueryInfo.h"

namespace suez_navi {

class Ha3QueryInfoR : public navi::Resource
{
public:
    Ha3QueryInfoR();
    ~Ha3QueryInfoR();
    Ha3QueryInfoR(const Ha3QueryInfoR &) = delete;
    Ha3QueryInfoR &operator=(const Ha3QueryInfoR &) = delete;
public:
    void def(navi::ResourceDefBuilder &builder) const override;
    bool config(navi::ResourceConfigContext &ctx) override;
    navi::ErrorCode init(navi::ResourceInitContext &ctx) override;
public:
    const isearch::common::QueryInfo &getQueryInfo() const;
public:
    static const std::string RESOURCE_ID;
private:
    isearch::common::QueryInfo _queryInfo;
};

NAVI_TYPEDEF_PTR(Ha3QueryInfoR);

}

