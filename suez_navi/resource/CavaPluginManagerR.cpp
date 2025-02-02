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
#include "suez_navi/resource/CavaPluginManagerR.h"

namespace suez_navi {

const std::string CavaPluginManagerR::RESOURCE_ID = "cava_plugin_manager_r";

CavaPluginManagerR::CavaPluginManagerR()
    : _metricReporterR(nullptr)
    , _cavaJitWrapperR(nullptr)
{
}

CavaPluginManagerR::~CavaPluginManagerR() {
}

void CavaPluginManagerR::def(navi::ResourceDefBuilder &builder) const {
    builder.name(RESOURCE_ID)
        .depend(MetricsReporterR::RESOURCE_ID, true,
                BIND_RESOURCE_TO(_metricReporterR))
        .depend(CavaJitWrapperR::RESOURCE_ID, true,
                BIND_RESOURCE_TO(_cavaJitWrapperR));
}

bool CavaPluginManagerR::config(navi::ResourceConfigContext &ctx) {
    ctx.Jsonize("function_infos", _functionInfos, _functionInfos);
    _cavaConfig.Jsonize(ctx);
    return true;
}

navi::ErrorCode CavaPluginManagerR::init(navi::ResourceInitContext &ctx) {
    suez::turing::CavaPluginManagerPtr manager(
        new suez::turing::CavaPluginManager(_cavaJitWrapperR->getCavaJitWrapper(),
                _metricReporterR->getReporter().get(), _cavaConfig._autoRegFuncPkgs));
    return navi::EC_NONE;
}

const suez::turing::CavaPluginManagerPtr &CavaPluginManagerR::getManager() const {
    return _manager;
}

REGISTER_RESOURCE(CavaPluginManagerR);

}
