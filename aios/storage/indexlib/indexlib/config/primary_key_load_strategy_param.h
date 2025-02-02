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
#ifndef __INDEXLIB_PRIMARY_KEY_LOAD_STRATEGY_PARAM_H
#define __INDEXLIB_PRIMARY_KEY_LOAD_STRATEGY_PARAM_H

#include <memory>

#include "indexlib/common_define.h"
#include "indexlib/index/primary_key/config/PrimaryKeyLoadStrategyParam.h"
#include "indexlib/indexlib.h"

namespace indexlib::config {
DEFINE_SHARED_PTR(PrimaryKeyLoadStrategyParam);
}

#endif //__INDEXLIB_PRIMARY_KEY_LOAD_STRATEGY_PARAM_H
