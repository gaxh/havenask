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
#include "navi/resource/MemoryPoolResourceBase.h"

namespace navi {

MemoryPoolResourceBase::MemoryPoolResourceBase() {
}

MemoryPoolResourceBase::~MemoryPoolResourceBase() {
    while (true) {
        autil::mem_pool::Pool *pool = nullptr;
        getPoolFromCache(pool);
        if (pool) {
            delete pool;
        } else {
            break;
        }
    }
}

size_t MemoryPoolResourceBase::getPoolFromCache(autil::mem_pool::Pool *&pool) {
    if (!_poolQueue.Pop(&pool)) {
        pool = nullptr;
    } else {
        pool->reset();
    }
    return _poolQueue.Size();
}

size_t MemoryPoolResourceBase::putPoolToCache(autil::mem_pool::Pool *pool) {
    _poolQueue.Push(pool);
    return _poolQueue.Size();
}

}
