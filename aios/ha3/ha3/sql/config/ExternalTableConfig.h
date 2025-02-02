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

#include "autil/legacy/jsonizable.h"
#include "aios/network/gig/multi_call/config/MultiCallConfig.h"

namespace multi_call {
class FlowControlConfig;
} // namespace multi_call

namespace isearch {
namespace sql {

class TableConfig : public autil::legacy::Jsonizable {
public:
    void Jsonize(autil::legacy::Jsonizable::JsonWrapper &json) override;

public:
    std::string schemaFile;
    std::string serviceName;
    std::string dbName;
    std::string tableName;
    bool allowDegradedAccess = false;
};

class GigConfig : public autil::legacy::Jsonizable {
public:
    void Jsonize(autil::legacy::Jsonizable::JsonWrapper &json) override;
public:
    multi_call::SubscribeConfig subscribeConfig;
    std::map<std::string, std::shared_ptr<multi_call::FlowControlConfig>> flowConfigMap;
};

class ServiceConfig : public autil::legacy::Jsonizable {
public:
    void Jsonize(autil::legacy::Jsonizable::JsonWrapper &json) override;

public:
    std::string type;
    std::string engineVersion;
    int64_t timeoutThresholdMs = 20;
    std::shared_ptr<autil::legacy::Jsonizable> miscConfig;
};

class Ha3ServiceMiscConfig : public autil::legacy::Jsonizable {
public:
    void Jsonize(autil::legacy::Jsonizable::JsonWrapper &json) override;
public:
    std::string authToken;
    std::string authKey;
    bool authEnabled = false;
};

class ExternalTableConfig : public autil::legacy::Jsonizable {
public:
    void Jsonize(autil::legacy::Jsonizable::JsonWrapper &json) override;

public:
    GigConfig gigConfig;
    std::map<std::string, ServiceConfig> serviceConfigMap;
    std::map<std::string, TableConfig> tableConfigMap;
};

} // namespace sql
} // namespace isearch
