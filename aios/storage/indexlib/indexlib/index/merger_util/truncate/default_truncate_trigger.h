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
#ifndef __INDEXLIB_DEFAULT_TRUNCATE_TRIGGER_H
#define __INDEXLIB_DEFAULT_TRUNCATE_TRIGGER_H

#include <memory>

#include "indexlib/common_define.h"
#include "indexlib/index/merger_util/truncate/truncate_trigger.h"
#include "indexlib/indexlib.h"

namespace indexlib::index::legacy {

class DefaultTruncateTrigger : public TruncateTrigger
{
public:
    DefaultTruncateTrigger(uint64_t truncThreshold);
    ~DefaultTruncateTrigger();

public:
    bool NeedTruncate(const TruncateTriggerInfo& info);

private:
    uint64_t mTruncThreshold;

private:
    IE_LOG_DECLARE();
};

DEFINE_SHARED_PTR(DefaultTruncateTrigger);
} // namespace indexlib::index::legacy

#endif //__INDEXLIB_DEFAULT_TRUNCATE_TRIGGER_H
