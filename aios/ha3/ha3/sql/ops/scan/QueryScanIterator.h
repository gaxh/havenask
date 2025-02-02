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
#include <stddef.h>
#include <stdint.h>
#include <vector>

#include "autil/Log.h"
#include "ha3/common/TimeoutTerminator.h"
#include "ha3/search/FilterWrapper.h"
#include "ha3/search/LayerMetas.h"
#include "ha3/search/QueryExecutor.h"
#include "ha3/sql/common/Log.h" // IWYU pragma: keep
#include "ha3/sql/ops/scan/ScanIterator.h"
#include "indexlib/indexlib.h"
#include "indexlib/misc/common.h"
#include "matchdoc/MatchDocAllocator.h"

namespace indexlib::index {
class DeletionMapReaderAdaptor;
}

namespace matchdoc {
class MatchDoc;
} // namespace matchdoc

DECLARE_REFERENCE_CLASS(index, DeletionMapReader);

namespace isearch {
namespace sql {

class QueryScanIterator : public ScanIterator {
public:
    QueryScanIterator(const search::QueryExecutorPtr &queryExecutor,
                      const search::FilterWrapperPtr &filterWrapper,
                      const matchdoc::MatchDocAllocatorPtr &matchDocAllocator,
                      const std::shared_ptr<indexlib::index::DeletionMapReaderAdaptor> &delMapReader,
                      const search::LayerMetaPtr &layerMeta,
                      common::TimeoutTerminator *_timeoutTerminator = NULL);
    virtual ~QueryScanIterator();

private:
    QueryScanIterator(const QueryScanIterator &);
    QueryScanIterator &operator=(const QueryScanIterator &);

public:
    autil::Result<bool> batchSeek(size_t batchSize,
                                  std::vector<matchdoc::MatchDoc> &matchDocs) override;
    uint32_t getTotalSeekDocCount() override;

private:
    inline bool tryToMakeItInRange(docid_t &docId);
    bool moveToCorrectRange(docid_t &docId);

    size_t batchFilter(const std::vector<int32_t> &docIds,
                       std::vector<matchdoc::MatchDoc> &matchDocs);

private:
    search::QueryExecutorPtr _queryExecutor;
    search::FilterWrapperPtr _filterWrapper;
    std::shared_ptr<indexlib::index::DeletionMapReaderAdaptor> _deletionMapReader;
    search::LayerMetaPtr _layerMeta;
    docid_t _curDocId;
    docid_t _curBegin;
    docid_t _curEnd;
    size_t _rangeCousor;
    int64_t _batchFilterTime;
    int64_t _scanTime;

private:
    AUTIL_LOG_DECLARE();
};

inline bool QueryScanIterator::tryToMakeItInRange(docid_t &docId) {
    if (_curEnd >= docId) {
        return true;
    }
    return moveToCorrectRange(docId);
}

typedef std::shared_ptr<QueryScanIterator> QueryScanIteratorPtr;
} // namespace sql
} // namespace isearch
