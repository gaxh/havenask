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
#ifndef __INDEXLIB_FILE_SYSTEM_CONFIG_H
#define __INDEXLIB_FILE_SYSTEM_CONFIG_H

#include <memory>

#include "autil/legacy/jsonizable.h"
#include "indexlib/common_define.h"
#include "indexlib/indexlib.h"

namespace indexlib { namespace config {

class FileSystemConfig : public autil::legacy::Jsonizable
{
public:
    FileSystemConfig();
    ~FileSystemConfig();

public:
    void Jsonize(autil::legacy::Jsonizable::JsonWrapper& json) override;

public:
    bool operator==(const FileSystemConfig& other) const;

public:
    // async read/write for merge
    uint32_t mReadThreadNum;
    uint32_t mReadQueueSize;
    uint32_t mWriteThreadNum;
    uint32_t mWriteQueueSize;

    // TODO: need remove
    // indexlib file system
    bool mForceFlushInMem;

public:
    static const uint32_t DEFAULT_READ_THREAD_NUM;
    static const uint32_t DEFAULT_READ_QUEUE_SIZE;
    static const uint32_t DEFAULT_WRITE_THREAD_NUM;
    static const uint32_t DEFAULT_WRITE_QUEUE_SIZE;

private:
    IE_LOG_DECLARE();
};

DEFINE_SHARED_PTR(FileSystemConfig);
}} // namespace indexlib::config

#endif //__INDEXLIB_FILE_SYSTEM_CONFIG_H
