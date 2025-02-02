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
#include "ha3/common/XMLResultFormatter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "alog/Logger.h"
#include "autil/ConstString.h"
#include "autil/Log.h"
#include "autil/StringUtil.h"
#include "autil/TimeUtility.h"
#include "ha3/common/AttributeItem.h"
#include "ha3/common/CommonDef.h"
#include "ha3/common/Hits.h"
#include "ha3/common/MultiErrorResult.h"
#include "ha3/common/SortExprMeta.h"
#include "ha3/common/SummaryHit.h"
#include "ha3/common/Tracer.h"
#include "ha3/config/HitSummarySchema.h"
#include "ha3/isearch.h"
#include "ha3/util/XMLFormatUtil.h"
#include "matchdoc/MatchDoc.h"
#include "matchdoc/MatchDocAllocator.h"
#include "matchdoc/Reference.h"
#include "suez/turing/expression/common.h"
#include "turing_ops_util/variant/SortExprMeta.h"

namespace isearch {
namespace common {
class ErrorResult;
}  // namespace common
}  // namespace isearch

using namespace std;
using namespace autil;
using namespace suez::turing;
namespace isearch {
namespace common {
AUTIL_LOG_SETUP(ha3, XMLResultFormatter);
using namespace isearch::util;

XMLResultFormatter::XMLResultFormatter() {
}

XMLResultFormatter::~XMLResultFormatter() {
}

std::string
XMLResultFormatter::xmlFormatResult(const ResultPtr &result)
{
    XMLResultFormatter formatter;
    stringstream ss;
    formatter.format(result, ss);
    return ss.str();
}

std::string
XMLResultFormatter::xmlFormatErrorResult(const ErrorResult &error)
{
    ResultPtr resultPtr(new common::Result);
    resultPtr->addErrorResult(error);
    return xmlFormatResult(resultPtr);
}

void XMLResultFormatter::
addCacheInfo(string &resultString, bool isFromCache) {
    string cacheInfo;
    if (isFromCache) {
        cacheInfo = "<fromCache>yes</fromCache>\n</Root>";
    } else {
        cacheInfo = "<fromCache>no</fromCache>\n</Root>";
    }
    StringUtil::replaceLast(resultString, "</Root>", cacheInfo);
}

void XMLResultFormatter::format(const ResultPtr &resultPtr,
                                stringstream &ss)
{
    fillHeader(ss);

    ss << "<Root>" << endl;
    formatTotalTime(resultPtr, ss);
    formatSortExpressionMeta(resultPtr, ss);
    formatHits(resultPtr, ss);
    formatAggregateResults(resultPtr, ss);
    formatErrorResult(resultPtr, ss);
    formatRequestTracer(resultPtr, ss);
    formatMeta(resultPtr, ss);
    ss << "</Root>" << endl;
}

void XMLResultFormatter::fillHeader(stringstream &ss) {
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
}


void XMLResultFormatter::formatSortExpressionMeta(
        const ResultPtr &resultPtr,
        stringstream &ss)
{
    const vector<SortExprMeta> &sortExprMetaVec = resultPtr->getSortExprMeta();
    string sortExprMetaStr;
    for (vector<SortExprMeta>::const_iterator it = sortExprMetaVec.begin();
         it != sortExprMetaVec.end(); ++it)
    {
        if (it->sortFlag) {
            sortExprMetaStr += "+";
        } else {
            sortExprMetaStr += "-";
        }
        sortExprMetaStr += it->sortExprName;
        sortExprMetaStr += ";";
    }
    if (!sortExprMetaStr.empty()) {
        sortExprMetaStr = sortExprMetaStr.substr(0, sortExprMetaStr.length() - 1);
    }
    ss << "<SortExprMeta>";
    XMLFormatUtil::formatCdataValue(sortExprMetaStr, ss);
    ss << "</SortExprMeta>\n";
}

void XMLResultFormatter::formatHits(const ResultPtr &resultPtr,
                                    stringstream &ss)
{
    Hits *hits = resultPtr->getHits();
    if (hits == NULL) {
        AUTIL_LOG(WARN, "There are no hits in the result");
        return;
    }
    uint32_t hitSize = hits->size();
    const common::Result::ClusterPartitionRanges &mergedRanges = resultPtr->getCoveredRanges();
    string coveredPercent = getCoveredPercentStr(mergedRanges);

    //add tag
    ss << "<hits " << "numhits=\"" << hitSize << "\"";
    ss << " totalhits=\"" << hits->totalHits() << "\"";
    ss << " coveredPercent=\"" << coveredPercent << "\">\n";
    for (uint32_t i = 0; i < hitSize; i++) {
        HitPtr hitPtr = hits->getHit(i);

        ss << "\t<hit";
        ss << " cluster_name=\"" << hitPtr->getClusterName() << "\"";
        ss << " hash_id=\"" << hitPtr->getHashId() << "\"";
        ss << " docid=\"" << hitPtr->getDocId() << "\"";
        if (hits->isIndependentQuery()) {
            ss << " gid=\"";
            hitPtr->toGid(ss);
            ss << "\"";
        }
        ss << ">" << endl;
        ss << "\t\t<fields>" << endl;
        SummaryHit *summaryHit = hitPtr->getSummaryHit();
        if (summaryHit) {
            const config::HitSummarySchema *hitSummarySchema = summaryHit->getHitSummarySchema();
            size_t summaryFieldCount = summaryHit->getFieldCount();
            for (size_t i = 0; i < summaryFieldCount; ++i)
            {
                const string &fieldName = hitSummarySchema->getSummaryFieldInfo(i)->fieldName;
                const StringView *str = summaryHit->getFieldValue(i);
                if (!str) {
                    continue;
                }
                ss << "\t\t\t<" << fieldName << ">";
                XMLFormatUtil::formatCdataValue(string(str->data(), str->size()), ss);
                ss << "</" << fieldName << ">\n";
            }
        }
        ss << "\t\t</fields>" << endl;

        ss << "\t\t<property>" << endl;
        const PropertyMap& propertyMap = hitPtr->getPropertyMap();
        for (PropertyMap::const_iterator it = propertyMap.begin();
             it != propertyMap.end(); ++it)
        {
            ss << "\t\t\t<" << it->first << ">";
            XMLFormatUtil::formatCdataValue(it->second, ss);
            ss << "</" << it->first << ">\n";
        }
        ss << "\t\t</property>" << endl;

        const AttributeMap& attributeMap = hitPtr->getAttributeMap();
        formatAttributeMap(attributeMap, "attribute", ss);

        const AttributeMap& variableValueMap = hitPtr->getVariableValueMap();
        formatAttributeMap(variableValueMap, "variableValue", ss);

        ss << "\t\t<sortExprValues>";
        for (uint32_t j = 0; j < hitPtr->getSortExprCount(); ++j) {
            if (j) {
                ss << "; ";
            }
            ss << hitPtr->getSortExprValue(j);
        }
        ss << "</sortExprValues>" << endl;

        Tracer *tracer = hitPtr->getTracer();
        if (tracer){
            ss << tracer->toXMLString();
        }
        const string &rawPk = hitPtr->getRawPk();
        if (!rawPk.empty()) {
            ss << "\t\t<raw_pk>";
            XMLFormatUtil::formatCdataValue(rawPk, ss);
            ss << "</raw_pk>" << endl;
        }
        ss << "\t</hit>\n";
    }

    const MetaHitMap& metaHitMap = hits->getMetaHitMap();
    for (MetaHitMap::const_iterator it = metaHitMap.begin();
         it != metaHitMap.end(); ++it)
    {
        const MetaHit &metaHit = it->second;
        if (metaHit.empty()) continue;
        ss << "\t<metaHit metaHitKey=\"" << it->first << "\">" << endl;
        for (MetaHit::const_iterator it2 = metaHit.begin();
             it2 != metaHit.end(); ++it2)
        {
            ss << "\t\t<" << it2->first << ">";
            XMLFormatUtil::formatCdataValue(it2->second, ss);
            ss << "</" << it2->first << ">\n";
        }

        ss << "\t</metaHit>\n";
    }

    ss << "</hits>\n";
    AUTIL_LOG(TRACE3, "after format hits\n%s", ss.str().c_str());
}

void XMLResultFormatter::formatMeta(const ResultPtr &resultPtr, stringstream &ss) {
    const MetaMap& metaMap = resultPtr->getMetaMap();
    for (MetaMap::const_iterator it = metaMap.begin();
         it != metaMap.end(); ++it)
    {
        const Meta &meta = it->second;
        if (meta.empty()) continue;
        ss << "<meta metaKey=\"" << it->first << "\">" << endl;
        for (Meta::const_iterator it2 = meta.begin();
             it2 != meta.end(); ++it2)
        {
            ss << "\t<" << it2->first << ">";
            XMLFormatUtil::formatCdataValue(it2->second, ss);
            ss << "</" << it2->first << ">\n";
        }

        ss << "</meta>\n";
    }
}
void XMLResultFormatter::formatAttributeMap(const AttributeMap &attributeMap,
        const std::string &tag,
        std::stringstream &ss)
{
    if (!attributeMap.empty()) {
        ss << "\t\t<" << tag << ">" << endl;
        for (AttributeMap::const_iterator it = attributeMap.begin();
             it != attributeMap.end(); ++it)
        {
            ss << "\t\t\t<" << it->first << ">";
            it->second->toXMLString(ss);
            ss << "</" << it->first << ">\n";
        }
        ss << "\t\t</" << tag << ">" << endl;
    }
}

string
XMLResultFormatter::getCoveredPercentStr(const common::Result::ClusterPartitionRanges &ranges) {
    double coveredPer = getCoveredPercent(ranges);
    char buf[10];
    sprintf(buf, "%.2lf", coveredPer);
    return string(buf);
}

void XMLResultFormatter::formatAggregateResults(const ResultPtr &resultPtr,
        stringstream &ss)
{
    ss << "<AggregateResults>" << endl;
    const AggregateResults& aggRes = resultPtr->getAggregateResults();
    for (AggregateResults::const_iterator it = aggRes.begin();
         it != aggRes.end(); it++)
    {
        AggregateResultPtr aggResultPtr = *it;
        assert(*it);
        ss << "\t<AggregateResult key='";
        XMLFormatUtil::escapeFiveXMLEntityReferences(aggResultPtr->getGroupExprStr(), ss);
        ss << "'>\n";
        formatAggregateResult(aggResultPtr, ss);
        ss << "\t</AggregateResult>" << endl;
    }
    ss << "</AggregateResults>" << endl;
}

void XMLResultFormatter::formatAggregateResult(const AggregateResultPtr &aggResultPtr,
        stringstream &ss)
{
    const auto &aggResultVector = aggResultPtr->getAggResultVector();
    auto allocatorPtr = aggResultPtr->getMatchDocAllocator();
    assert(allocatorPtr);
    auto ref = allocatorPtr->findReferenceWithoutType(common::GROUP_KEY_REF);
    assert(ref);
    auto referenceVec = allocatorPtr->getAllNeedSerializeReferences(SL_QRS);
    uint32_t aggFunCount = aggResultPtr->getAggFunCount();
    assert(aggFunCount + 1 == referenceVec.size());
    for (matchdoc::MatchDoc aggFunResults : aggResultVector) {
        const string &aggExprValue = ref->toString(aggFunResults);
        ss << "\t\t<group value='";
        XMLFormatUtil::escapeFiveXMLEntityReferences(aggExprValue, ss);
        ss << "'>";
        for (uint32_t i = 0; i < aggFunCount; i++) {
            ss << '<' << aggResultPtr->getAggFunName(i) << '>';
            ss << referenceVec[i + 1]->toString(aggFunResults);
            ss << "</" << aggResultPtr->getAggFunName(i) << '>';
        }
        ss << "</group>\n";
    }
}

void XMLResultFormatter::formatErrorResult(const ResultPtr &resultPtr,
        stringstream &ss)
{
    ss << resultPtr->getMultiErrorResult()->toXMLString();
}

void XMLResultFormatter::formatTotalTime(const ResultPtr &resultPtr,
        stringstream &ss)
{
    float totalTime = ((float)(resultPtr->getTotalTime()/1000))/((float)1000);
    char timeStr[16];
    sprintf(timeStr, "%.3f", totalTime);
    ss << "<TotalTime>" << timeStr << "</TotalTime>" << endl;
}

void XMLResultFormatter::formatRequestTracer(const ResultPtr &resultPtr,
        stringstream &ss)
{
    Tracer* tracer = resultPtr->getTracer();
    if (tracer){
        ss << "<Request_Trace>\n";
        ss << tracer->toXMLString();
        ss << "</Request_Trace>\n";
    }
}

} // namespace common
} // namespace isearch
