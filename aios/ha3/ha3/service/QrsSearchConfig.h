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
#include <set>
#include <string>

#include "autil/CompressionUtil.h"
#include "autil/Log.h" // IWYU pragma: keep
#include "ha3/qrs/QrsChainManager.h"

namespace isearch {
namespace service {

class QrsSearchConfig
{
public:
    QrsSearchConfig();
    ~QrsSearchConfig();
private:
    QrsSearchConfig(const QrsSearchConfig &);
    QrsSearchConfig& operator=(const QrsSearchConfig &);
public:
    std::string _configDir;
    qrs::QrsChainManagerPtr _qrsChainMgrPtr;
    autil::CompressType _resultCompressType;
    std::set<std::string> _metricsSrcWhiteList;
private:
    AUTIL_LOG_DECLARE();
};

typedef std::shared_ptr<QrsSearchConfig> QrsSearchConfigPtr;

} // namespace service
} // namespace isearch
