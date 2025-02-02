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
#include "indexlib/table/index_task/merger/EndMergeTaskOperation.h"

#include "indexlib/file_system/Directory.h"
#include "indexlib/file_system/JsonUtil.h"
#include "indexlib/framework/TabletData.h"
#include "indexlib/framework/Version.h"
#include "indexlib/framework/VersionLoader.h"
#include "indexlib/framework/index_task/MergeTaskDefine.h"
#include "indexlib/table/index_task/IndexTaskConstant.h"

namespace indexlibv2::table {
AUTIL_LOG_SETUP(indexlib.table, EndMergeTaskOperation);

EndMergeTaskOperation::EndMergeTaskOperation(const framework::IndexOperationDescription& desc)
    : framework::IndexOperation(desc.GetId(), desc.UseOpFenceDir())
    , _desc(desc)
{
}

EndMergeTaskOperation::~EndMergeTaskOperation() {}

Status EndMergeTaskOperation::Execute(const framework::IndexTaskContext& context)
{
    std::string versionStr;
    if (!_desc.GetParameter(PARAM_TARGET_VERSION, versionStr)) {
        AUTIL_LOG(ERROR, "get target version from desc failed");
        return Status::Corruption("get target version from desc failed");
    }
    framework::Version lastVersion;
    RETURN_IF_STATUS_ERROR(indexlib::file_system::JsonUtil::FromString(versionStr, &lastVersion).Status(),
                           "parse version [%s] failed", versionStr.c_str());
    auto versionId = lastVersion.GetVersionId();

    framework::Version version;
    auto indexRoot = context.GetIndexRoot();
    RETURN_IF_STATUS_ERROR(framework::VersionLoader::GetVersion(indexRoot, versionId, &version),
                           "load version [%d] from [%s]", versionId, indexRoot->GetRootDir().c_str());

    framework::MergeResult mergeResult;
    mergeResult.baseVersionId = context.GetTabletData()->GetOnDiskVersion().GetVersionId();
    mergeResult.targetVersionId = version.GetVersionId();
    std::string resultStr;
    RETURN_IF_STATUS_ERROR(indexlib::file_system::JsonUtil::ToString(mergeResult, &resultStr).Status(),
                           "jsonize merge result failed for version [%d]", mergeResult.targetVersionId);
    context.SetResult(std::move(resultStr));
    AUTIL_LOG(INFO, "End Merge Operation done");
    return Status::OK();
}

framework::IndexOperationDescription
EndMergeTaskOperation::CreateOperationDescription(framework::IndexOperationId opId, const framework::Version& version)
{
    framework::IndexOperationDescription endTaskOpDesc(opId, framework::END_TASK_OPERATION);
    std::string versionContent;
    auto status = indexlib::file_system::JsonUtil::ToString(version, &versionContent).Status();
    assert(status.IsOK());
    endTaskOpDesc.AddParameter(PARAM_TARGET_VERSION, versionContent);
    return endTaskOpDesc;
}
} // namespace indexlibv2::table
