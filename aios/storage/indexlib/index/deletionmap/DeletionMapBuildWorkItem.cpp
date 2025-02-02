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
#include "indexlib/index/deletionmap/DeletionMapBuildWorkItem.h"

namespace indexlib::index {
AUTIL_LOG_SETUP(indexlib.index, DeletionMapBuildWorkItem);

DeletionMapBuildWorkItem::DeletionMapBuildWorkItem(SingleDeletionMapBuilder* builder,
                                                   indexlibv2::document::IDocumentBatch* documentBatch)
    : BuildWorkItem(("_DELETION_MAP_"), BuildWorkItemType::DELETION_MAP, documentBatch)
    , _builder(builder)
{
}

DeletionMapBuildWorkItem::~DeletionMapBuildWorkItem() {}

Status DeletionMapBuildWorkItem::doProcess()
{
    for (size_t i = 0; i < _documentBatch->GetBatchSize(); ++i) {
        std::shared_ptr<indexlibv2::document::IDocument> iDoc = (*_documentBatch)[i];
        DocOperateType opType = iDoc->GetDocOperateType();
        assert(opType != UNKNOWN_OP);
        if (opType == ADD_DOC) {
            RETURN_STATUS_DIRECTLY_IF_ERROR(_builder->AddDocument(iDoc.get()));
        } else if (opType == DELETE_DOC) {
            RETURN_STATUS_DIRECTLY_IF_ERROR(_builder->DeleteDocument(iDoc.get()));
        } else {
            continue;
        }
    }
    return Status::OK();
}

} // namespace indexlib::index