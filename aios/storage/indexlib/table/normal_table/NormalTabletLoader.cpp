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
#include "indexlib/table/normal_table/NormalTabletLoader.h"

#include <any>

#include "indexlib/config/TabletSchema.h"
#include "indexlib/document/document_rewriter/DocumentInfoToAttributeRewriter.h"
#include "indexlib/file_system/Directory.h"
#include "indexlib/framework/ResourceMap.h"
#include "indexlib/framework/TabletData.h"
#include "indexlib/index/IIndexFactory.h"
#include "indexlib/index/IIndexReader.h"
#include "indexlib/index/IndexFactoryCreator.h"
#include "indexlib/index/IndexerParameter.h"
#include "indexlib/index/deletionmap/DeletionMapModifier.h"
#include "indexlib/index/operation_log/DoNothingRedoStrategy.h"
#include "indexlib/index/operation_log/OperationCursor.h"
#include "indexlib/index/operation_log/OperationLogIndexReader.h"
#include "indexlib/index/operation_log/OperationLogReplayer.h"
#include "indexlib/index/operation_log/TargetSegmentsRedoStrategy.h"
#include "indexlib/index/primary_key/PrimaryKeyReader.h"
#include "indexlib/table/BuiltinDefine.h"
#include "indexlib/table/normal_table/Common.h"
#include "indexlib/table/normal_table/NormalTabletModifier.h"
#include "indexlib/table/normal_table/NormalTabletPatcher.h"
#include "indexlib/table/normal_table/virtual_attribute/VirtualAttributeIndexFactory.h"
#include "indexlib/table/normal_table/virtual_attribute/VirtualAttributeIndexReader.h"

namespace indexlibv2::table {
AUTIL_LOG_SETUP(indexlib.table, NormalTabletLoader);
#define TABLET_LOG(level, format, args...) AUTIL_LOG(level, "[%s] [%p] " format, _tabletName.c_str(), this, ##args)

NormalTabletLoader::NormalTabletLoader(const std::string& fenceName, bool reopenDataConsistent, bool loadIndexForCheck)
    : CommonTabletLoader(fenceName)
    , _reopenDataConsistent(reopenDataConsistent)
    , _loadIndexForCheck(loadIndexForCheck)
{
}

using RedoParam =
    std::tuple<Status, std::unique_ptr<indexlib::index::OperationLogReplayer>, std::unique_ptr<NormalTabletModifier>,
               std::unique_ptr<indexlib::index::PrimaryKeyIndexReader>>;
RedoParam NormalTabletLoader::CreateRedoParameters(const framework::TabletData& tabletData) const
{
    // prepare operatonReplayer
    indexlib::index::OperationLogIndexReader operationIndexReader;
    auto opConfig = _schema->GetIndexConfig(indexlib::index::OPERATION_LOG_INDEX_TYPE_STR,
                                            indexlib::index::OPERATION_LOG_INDEX_NAME);
    // assert(opConfig);
    auto status = operationIndexReader.Open(opConfig, &tabletData);
    if (!status.IsOK()) {
        TABLET_LOG(ERROR, "open operation index reader failed");
        return {status, nullptr, nullptr, nullptr};
    }
    auto opReplayer = operationIndexReader.CreateReplayer();
    assert(opReplayer);

    // prepare modifier
    auto modifier = std::make_unique<NormalTabletModifier>();
    status = modifier->Init(_schema, tabletData, /*deleteInBranch=*/true, /*op2patchdir*/ nullptr);
    if (!status.IsOK()) {
        TABLET_LOG(ERROR, "init modifier failed");
        return {status, nullptr, nullptr, nullptr};
    }

    // prepare pkReader
    auto [createStatus, pkIndexFactory] =
        index::IndexFactoryCreator::GetInstance()->Create(index::PRIMARY_KEY_INDEX_TYPE_STR);
    if (!createStatus.IsOK()) {
        TABLET_LOG(ERROR, "create PrimaryKeyIndexFactory failed, %s", createStatus.ToString().c_str());
        return {createStatus, nullptr, nullptr, nullptr};
    }
    index::IndexerParameter indexParam;
    auto pkConfigs = _schema->GetIndexConfigs(index::PRIMARY_KEY_INDEX_TYPE_STR);
    assert(1 == pkConfigs.size());
    auto originPkConfig = std::dynamic_pointer_cast<index::PrimaryKeyIndexConfig>(pkConfigs[0]);
    std::shared_ptr<index::PrimaryKeyIndexConfig> pkConfig(originPkConfig->Clone());
    pkConfig->DisablePrimaryKeyCombineSegments();
    auto pkReader = pkIndexFactory->CreateIndexReader(pkConfig, indexParam);
    assert(pkReader);

    std::unique_ptr<indexlib::index::PrimaryKeyIndexReader> typedPkReader(
        dynamic_cast<indexlib::index::PrimaryKeyIndexReader*>(pkReader.release()));
    if (!typedPkReader) {
        TABLET_LOG(ERROR, "cast pkReader failed");
        return {Status::InvalidArgs("cast pkReader failed"), nullptr, nullptr, nullptr};
    }

    status = typedPkReader->OpenWithoutPKAttribute(pkConfig, &tabletData);
    if (!status.IsOK()) {
        TABLET_LOG(ERROR, "open pkReader failed");
        return {status, nullptr, nullptr, nullptr};
    }
    return {Status::OK(), std::move(opReplayer), std::move(modifier), std::move(typedPkReader)};
}

Status NormalTabletLoader::RemoveObsoleteRtDocs(
    const framework::Locator& versionLocator,
    const std::vector<std::pair<std::shared_ptr<framework::Segment>, docid_t>>& segments,
    const framework::TabletData& newTabletData)
{
    TABLET_LOG(INFO, "remove obsolete rt docs begin");
    if (segments.empty()) {
        TABLET_LOG(INFO, "needn't reclaim, segment empty");
        return Status::OK();
    }
    auto createReader = [this, &newTabletData](const std::string& fieldName) -> std::shared_ptr<index::IIndexReader> {
        VirtualAttributeIndexFactory builtinAttributeIndexFactory;
        index::IndexerParameter indexParam;
        auto config = _schema->GetIndexConfig(VIRTUAL_ATTRIBUTE_INDEX_TYPE_STR, fieldName);
        auto reader = builtinAttributeIndexFactory.CreateIndexReader(config, indexParam);
        auto status = reader->Open(config, &newTabletData);
        if (!status.IsOK()) {
            TABLET_LOG(INFO, "open reader failed, field name[%s]", fieldName.c_str());
            return nullptr;
        }
        return std::shared_ptr<index::IIndexReader>(reader.release());
    };
    index::DeletionMapModifier deletionMapModifier;
    auto deletionMapConfig =
        _schema->GetIndexConfig(index::DELETION_MAP_INDEX_TYPE_STR, index::DELETION_MAP_INDEX_NAME);
    if (!deletionMapConfig) {
        TABLET_LOG(ERROR, "get deletion map config failed");
        return Status::Corruption("get deletion map config failed");
    }
    RETURN_IF_STATUS_ERROR(deletionMapModifier.Open(deletionMapConfig, &newTabletData),
                           "open deletion map modifier failed");

    auto timestampReader = std::dynamic_pointer_cast<VirtualAttributeIndexReader<int64_t>>(
        createReader(document::DocumentInfoToAttributeRewriter::VIRTUAL_TIMESTAMP_FIELD_NAME));
    if (timestampReader == nullptr) {
        TABLET_LOG(ERROR, "create timestamp reader failed");
        return Status::InvalidArgs("create timestamp reader failed");
    }
    auto hashIdReader = std::dynamic_pointer_cast<VirtualAttributeIndexReader<uint16_t>>(
        createReader(document::DocumentInfoToAttributeRewriter::VIRTUAL_HASH_ID_FIELD_NAME));
    if (hashIdReader == nullptr) {
        TABLET_LOG(ERROR, "create hasid reader failed");
        return Status::InvalidArgs("create hash id reader failed");
    }
    size_t removedDocCount = 0;
    for (const auto& [segment, baseDocId] : segments) {
        size_t segmentRemovedCount = 0;
        for (docid_t docId = 0; docId < segment->GetSegmentInfo()->docCount; docId++) {
            uint16_t hashId;
            int64_t timestamp;
            docid_t globalDocId = baseDocId + docId;
            if (!hashIdReader->Seek(globalDocId, hashId)) {
                TABLET_LOG(INFO, "get hash id failed, docId[%d]]", globalDocId);
                return Status::InvalidArgs("get hash id failed");
            }

            if (!timestampReader->Seek(globalDocId, timestamp)) {
                TABLET_LOG(INFO, "get timestamp failed, docId[%d]", globalDocId);
                return Status::InvalidArgs("get hash id failed");
            }
            auto result = versionLocator.IsFasterThan(hashId, timestamp);
            assert(result != framework::Locator::LocatorCompareResult::LCR_PARTIAL_FASTER);
            if (result == framework::Locator::LocatorCompareResult::LCR_INVALID) {
                TABLET_LOG(ERROR, "locator compare failed, hash id[%u]", hashId);
                return Status::InvalidArgs("locator compare failed, invalid hash id");
            }
            if (result == framework::Locator::LocatorCompareResult::LCR_FULLY_FASTER) {
                RETURN_IF_STATUS_ERROR(deletionMapModifier.DeleteInBranch(globalDocId),
                                       "delete global docid [%d] in branch failed", globalDocId);
                removedDocCount++;
                segmentRemovedCount++;
            }
        }
        TABLET_LOG(INFO, "remove [%lu] doc from segment [%d]", segmentRemovedCount, segment->GetSegmentId());
    }
    TABLET_LOG(INFO, "remove obsolete rt docs end, total remove [%lu] docs", removedDocCount);
    return Status::OK();
}

std::pair<Status, bool> NormalTabletLoader::IsRtFullyFasterThanInc(const framework::TabletData& lastTabletData,
                                                                   const framework::Version& newOnDiskVersion)
{
    framework::Locator rtLocator = lastTabletData.GetLocator();
    auto newOnDiskVersionLocator = newOnDiskVersion.GetLocator();
    if (rtLocator.IsValid() && !newOnDiskVersionLocator.IsSameSrc(rtLocator, true)) {
        TABLET_LOG(INFO, "src not same rtLocator [%s], incLocator [%s]", rtLocator.DebugString().c_str(),
                   newOnDiskVersion.GetLocator().DebugString().c_str());
        return {Status::OK(), false};
    }
    if (!_isOnline) {
        // when offline parallel build, reopen version range bigger than current version range
        return {Status::OK(), true};
    }
    auto result = rtLocator.IsFasterThan(newOnDiskVersionLocator, true);
    TABLET_LOG(INFO, "rtLocator [%s], incLocator [%s]", rtLocator.DebugString().c_str(),
               newOnDiskVersion.GetLocator().DebugString().c_str());
    if (result == framework::Locator::LocatorCompareResult::LCR_FULLY_FASTER) {
        TABLET_LOG(INFO, "realtime is faster");
        return {Status::OK(), true};
    }
    TABLET_LOG(INFO, "increment is faster");
    return {Status::OK(), false};
}

static Status ValidatePreloadParams(const std::vector<std::shared_ptr<framework::Segment>>& newOnDiskVersionSegments,
                                    const framework::Version& newOnDiskVersion)
{
    if (newOnDiskVersionSegments.size() != newOnDiskVersion.GetSegmentCount()) {
        return Status::InvalidArgs("new segments count is not equal to new version's segment count ",
                                   newOnDiskVersionSegments.size(), " vs ", newOnDiskVersion.GetSegmentCount());
    }
    size_t segmentIdx = 0;
    for (auto [segId, _] : newOnDiskVersion) {
        if (segId != newOnDiskVersionSegments[segmentIdx]->GetSegmentId()) {
            return Status::InvalidArgs("version segment id ", segId, " and ",
                                       newOnDiskVersionSegments[segmentIdx]->GetSegmentId(), " inconsistent");
        }
        ++segmentIdx;
    }
    return Status::OK();
}

Status NormalTabletLoader::DoPreLoad(const framework::TabletData& lastTabletData, Segments newOnDiskVersionSegments,
                                     const framework::Version& newOnDiskVersion)
{
    assert(_schema);
    assert(_schema->GetTableType() == indexlib::table::TABLE_TYPE_NORMAL);
    RETURN_IF_STATUS_ERROR(ValidatePreloadParams(newOnDiskVersionSegments, newOnDiskVersion), "params invalid");
    _segmentIdsOnPreload.clear();
    _newVersion = newOnDiskVersion.Clone();
    _tabletName = lastTabletData.GetTabletName();
    _newSegments.clear();
    TABLET_LOG(INFO, "do preload for table begin");

    auto [status, fullyFaster] = IsRtFullyFasterThanInc(lastTabletData, _newVersion);
    RETURN_IF_STATUS_ERROR(status, "failed to compare rt locator and inc locator");
    // collect new version segments
    docid_t baseDocId = 0;
    _newSegments.insert(_newSegments.end(), newOnDiskVersionSegments.begin(), newOnDiskVersionSegments.end());
    for (const auto& segment : newOnDiskVersionSegments) {
        baseDocId += segment->GetSegmentInfo()->docCount;
    }
    std::vector<std::pair<std::shared_ptr<framework::Segment>, docid_t>> partialReclaimRtSegments;
    if (!fullyFaster) {
        TABLET_LOG(INFO, "rt is not fully faster than inc, need drop rt");
        _dropRt = true;
        _resourceMap = lastTabletData.GetResourceMap()->Clone();
    } else {
        TABLET_LOG(INFO, "rt is fully faster than inc, keep rt segments");
        auto lastOnDiskSegmentId = INVALID_SEGMENTID;
        if (!newOnDiskVersionSegments.empty()) {
            lastOnDiskSegmentId = (*newOnDiskVersionSegments.rbegin())->GetSegmentId();
        }
        auto slice = lastTabletData.CreateSlice();
        for (const auto& oldSegment : slice) {
            _segmentIdsOnPreload.insert(oldSegment->GetSegmentId());
            auto [status, segLocator] = oldSegment->GetLastLocator();
            assert(status.IsOK()); // mem segment will not deserialize failed
            if (!NeedReclaimFullSegment(_newVersion.GetLocator(), lastOnDiskSegmentId, oldSegment)) {
                TABLET_LOG(INFO, "reserve segment [%d]", oldSegment->GetSegmentId());
                _newSegments.push_back(oldSegment);
                partialReclaimRtSegments.emplace_back(std::make_pair(oldSegment, baseDocId));
                baseDocId += oldSegment->GetSegmentInfo()->docCount;
            }
        }
        framework::TabletData newTabletData(_tabletName);
        RETURN_IF_STATUS_ERROR(
            newTabletData.Init(_newVersion.Clone(), _newSegments, lastTabletData.GetResourceMap()->Clone()),
            "tablet data init fail");
        if (_isOnline) {
            RETURN_IF_STATUS_ERROR(
                RemoveObsoleteRtDocs(_newVersion.GetLocator(), partialReclaimRtSegments, newTabletData),
                "create reclaim resource failed");
            RETURN_IF_STATUS_ERROR(
                PatchAndRedo(newOnDiskVersion, newOnDiskVersionSegments, lastTabletData, newTabletData),
                "redo operation and patch failed");
        } else {
            TABLET_LOG(INFO, "skip patch and redo for offline");
        }
        _resourceMap = newTabletData.GetResourceMap()->Clone();
    }
    return Status::OK();
}

bool NormalTabletLoader::NeedReclaimFullSegment(const framework::Locator& versionLocator,
                                                segmentid_t lastOnDiskSegmentId,
                                                const std::shared_ptr<framework::Segment> segment) const
{
    // TODO(xiaohao.yxh) reclaim segment with no operation log and valid doc
    if (_newVersion.HasSegment(segment->GetSegmentId())) {
        return true;
    }

    schemaid_t segSchemaId = segment->GetSegmentSchema()->GetSchemaId();
    if (segSchemaId != INVALID_SCHEMAID && _newVersion.GetReadSchemaId() != INVALID_SCHEMAID) {
        if (segSchemaId < _newVersion.GetReadSchemaId()) {
            TABLET_LOG(INFO,
                       "reclaim status[%d] segment [%d], segment schema id[%u] is"
                       " smaller than new version read schema id[%u].",
                       (int)(segment->GetSegmentStatus()), segment->GetSegmentId(), segSchemaId,
                       _newVersion.GetReadSchemaId());
            return true;
        }
    }

    auto [status, segLocator] = segment->GetLastLocator();
    assert(status.IsOK());
    if (!versionLocator.IsSameSrc(segLocator, true)) {
        TABLET_LOG(INFO, "segment[%d] locator [%s] src not equal with new version locator [%s] src",
                   segment->GetSegmentId(), segLocator.DebugString().c_str(), versionLocator.DebugString().c_str());
        return true;
    }
    auto compareResult = versionLocator.IsFasterThan(segLocator, true);
    if (compareResult == framework::Locator::LocatorCompareResult::LCR_FULLY_FASTER) {
        TABLET_LOG(INFO, "reclaim segment [%d]", segment->GetSegmentId());
        return true;
    }
    return false;
}
Status NormalTabletLoader::PatchAndRedo(const framework::Version& newOnDiskVersion, Segments newOnDiskVersionSegments,
                                        const framework::TabletData& lastTabletData,
                                        const framework::TabletData& newTabletData)
{
    _targetRedoSegments.clear();
    std::set<segmentid_t> diffDiskSegmentIds;
    std::vector<std::shared_ptr<framework::Segment>> diffDiskSegments;

    for (auto [segmentId, _] : newOnDiskVersion) {
        auto seg = lastTabletData.GetSegment(segmentId);
        if (seg != nullptr && seg->GetSegmentStatus() == framework::Segment::SegmentStatus::ST_BUILT) {
            continue;
        }
        // TODO(xiaohao.yxh) dumping?
        diffDiskSegmentIds.insert(segmentId);
    }
    if (diffDiskSegmentIds.empty()) {
        return Status::OK();
    }
    for (auto segment : newOnDiskVersionSegments) {
        if (diffDiskSegmentIds.find(segment->GetSegmentId()) != diffDiskSegmentIds.end()) {
            diffDiskSegments.push_back(segment);
        }
    }

    auto [paramStatus, opReplayer, modifier, pkReader] = CreateRedoParameters(newTabletData);
    RETURN_IF_STATUS_ERROR(paramStatus, "prepare redo params failed");

    TABLET_LOG(INFO, "load patch begin");
    RETURN_IF_STATUS_ERROR(
        NormalTabletPatcher::LoadPatch(diffDiskSegments, newTabletData, _schema, nullptr /*op2patch*/, modifier.get()),
        "load patch failed");
    TABLET_LOG(INFO, "load patch done");

    _opCursor.reset(new indexlib::index::OperationCursor);
    auto slice = newTabletData.CreateSlice();
    segmentid_t minSegmentId = *(diffDiskSegmentIds.begin());
    for (auto segment : slice) {
        auto segmentId = segment->GetSegmentId();
        if (segmentId < minSegmentId) {
            continue;
        }
        // for offline segment , data may not consistent, need redo for all segments
        bool reopenDataConsistent = segmentId & framework::Segment::PRIVATE_SEGMENT_ID_MASK || _reopenDataConsistent;

        std::shared_ptr<indexlib::index::OperationRedoStrategy> redoStrategy;
        if (_loadIndexForCheck) {
            redoStrategy = std::make_shared<indexlib::index::DoNothingRedoStrategy>();
        } else {
            redoStrategy = std::make_shared<indexlib::index::TargetSegmentsRedoStrategy>(reopenDataConsistent);
        }

        RETURN_IF_STATUS_ERROR(redoStrategy->Init(&newTabletData, segmentId, _targetRedoSegments),
                               "init redo strategy failed");

        indexlib::index::OperationLogReplayer::RedoParams redoParams = {
            pkReader.get(), modifier.get(), redoStrategy.get(), _memoryQuotaController.get()};
        framework::Locator redoLocator;
        if (diffDiskSegmentIds.find(segmentId) == diffDiskSegmentIds.end() &&
            (segmentId & framework::Segment::PRIVATE_SEGMENT_ID_MASK)) {
            // inc segment do all patch to inc segment
            // rt segment do patch after version locator
            redoLocator = _newVersion.GetLocator();
        }
        auto [status, cursor] = opReplayer->RedoOperationsFromOneSegment(redoParams, segmentId, redoLocator);

        RETURN_IF_STATUS_ERROR(status, "redo operation failed");
        (*_opCursor) = cursor;
        if (diffDiskSegmentIds.find(segmentId) != diffDiskSegmentIds.end()) {
            TABLET_LOG(INFO, "add segment [%d] to target redo segments", segmentId);
            _targetRedoSegments.push_back(segmentId);
        }
    }
    return Status::OK();
}

std::pair<Status, std::unique_ptr<framework::TabletData>>
NormalTabletLoader::FinalLoad(const framework::TabletData& currentTabletData)
{
    TABLET_LOG(INFO, "final load begin");
    assert(_schema->GetTableType() == indexlib::table::TABLE_TYPE_NORMAL);
    auto tabletData = std::make_unique<framework::TabletData>(_tabletName);
    if (!_dropRt) {
        auto slice = currentTabletData.CreateSlice();
        // add new mem segments
        for (const auto& segment : slice) {
            if (_segmentIdsOnPreload.find(segment->GetSegmentId()) == _segmentIdsOnPreload.end()) {
                TABLET_LOG(INFO, "add segment [%d] at final load", segment->GetSegmentId());
                _newSegments.emplace_back(segment);
            }
        }
        RETURN2_IF_STATUS_ERROR(tabletData->Init(_newVersion.Clone(), std::move(_newSegments), std::move(_resourceMap)),
                                nullptr, "new tablet data init failed, version[%s]", _newVersion.ToString().c_str());
        if (_targetRedoSegments.empty()) {
            return std::make_pair(Status::OK(), std::move(tabletData));
        }
        auto [paramStatus, opReplayer, modifier, pkReader] = CreateRedoParameters(*tabletData);
        RETURN2_IF_STATUS_ERROR(paramStatus, nullptr, "prepare redo params failed");

        // final load is to replay rt op, this scene think data is consistent,
        // op only to diff segment, no need for all segment
        std::shared_ptr<indexlib::index::OperationRedoStrategy> redoStrategy;
        if (_loadIndexForCheck) {
            redoStrategy = std::make_shared<indexlib::index::DoNothingRedoStrategy>();
        } else {
            redoStrategy =
                std::make_shared<indexlib::index::TargetSegmentsRedoStrategy>(/*isIncConsistentWithRealtime*/ true);
        }

        RETURN2_IF_STATUS_ERROR(redoStrategy->Init(tabletData.get(), _opCursor->segId, _targetRedoSegments), nullptr,
                                "init redo strategy failed");
        indexlib::index::OperationLogReplayer::RedoParams redoParams = {
            pkReader.get(), modifier.get(), redoStrategy.get(), _memoryQuotaController.get()};
        auto [redoStatus, cursor] =
            opReplayer->RedoOperationsFromCursor(redoParams, *_opCursor, _newVersion.GetLocator());
        RETURN2_IF_STATUS_ERROR(redoStatus, nullptr, "redo failed");
    } else {
        RETURN2_IF_STATUS_ERROR(tabletData->Init(_newVersion.Clone(), _newSegments, std::move(_resourceMap)), nullptr,
                                "new tablet data init failed, version[%s]", _newVersion.ToString().c_str());
        framework::TabletData emptyTabletData(_tabletName);
        if (_isOnline) {
            RETURN2_IF_STATUS_ERROR(PatchAndRedo(_newVersion, std::move(_newSegments), emptyTabletData, *tabletData),
                                    nullptr, "redo operation and patch failed");
        }
    }

    TABLET_LOG(INFO, "final load success");
    return make_pair(Status::OK(), std::move(tabletData));
}

size_t NormalTabletLoader::EstimateMemUsed(const std::shared_ptr<config::TabletSchema>& schema,
                                           const std::vector<framework::Segment*>& segments)
{
    size_t totalMemUsed = 0;
    auto indexConfigs = schema->GetIndexConfigs();
    for (const auto& indexConfig : indexConfigs) {
        auto indexType = indexConfig->GetIndexType();
        if (index::PRIMARY_KEY_INDEX_TYPE_STR == indexType) {
            auto pkConfig = std::dynamic_pointer_cast<indexlibv2::index::PrimaryKeyIndexConfig>(indexConfig);
            assert(pkConfig);
            auto pkType = pkConfig->GetInvertedIndexType();
            try {
                if (it_primarykey64 == pkType) {
                    index::PrimaryKeyReader<uint64_t> pkReader(nullptr /*metric*/);
                    totalMemUsed += pkReader.EstimateLoadSize(segments, indexConfig);
                } else if (it_primarykey128 == pkType) {
                    index::PrimaryKeyReader<autil::uint128_t> pkReader(nullptr /*metric*/);
                    totalMemUsed += pkReader.EstimateLoadSize(segments, indexConfig);
                } else {
                    assert(false); // never got hear
                }
            } catch (...) {
                // TODO: if exception occurs, how-to?
                TABLET_LOG(ERROR, "fail to estimate memory size for primary key [%s].",
                           pkConfig->GetIndexName().c_str());
            }
        }
    }

    for (auto segment : segments) {
        totalMemUsed += segment->EstimateMemUsed(schema);
    }

    return totalMemUsed;
}

} // namespace indexlibv2::table
