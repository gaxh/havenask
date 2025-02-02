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
#ifndef __INDEXLIB_KKV_READER_IMPL_FACTORY_H
#define __INDEXLIB_KKV_READER_IMPL_FACTORY_H

#include <memory>

#include "indexlib/codegen/codegen_info.h"
#include "indexlib/codegen/codegen_object.h"
#include "indexlib/common_define.h"
#include "indexlib/index/kkv/kkv_reader_impl.h"
#include "indexlib/indexlib.h"

namespace indexlib { namespace index {

class KKVReaderImplFactory
{
public:
    KKVReaderImplFactory();
    ~KKVReaderImplFactory();

public:
    static KKVReaderImplBase* Create(const config::KKVIndexConfigPtr& kkvConfig, const std::string& schemaName,
                                     std::string& readerType);
    static void CollectAllCode(codegen::CodegenInfo& codegenInfo)
    {
        PARTICIPATE_CODEGEN(KVReaderImplFactory, codegenInfo);
    }
};

DEFINE_SHARED_PTR(KKVReaderImplFactory);
}} // namespace indexlib::index

#endif //__INDEXLIB_KKV_READER_IMPL_FACTORY_H
