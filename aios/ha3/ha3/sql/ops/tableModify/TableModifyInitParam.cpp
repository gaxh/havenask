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
#include "ha3/sql/ops/tableModify/TableModifyInitParam.h"
#include "ha3/sql/ops/util/KernelUtil.h"

#include "navi/log/NaviLogger.h"

using namespace std;

namespace isearch {
namespace sql {

bool TableModifyInitParam::initFromJson(navi::KernelConfigContext &ctx) {
    try {
        ctx.Jsonize("catalog_name", catalogName);
        ctx.Jsonize("db_name", dbName);
        ctx.Jsonize("table_name", tableName);
        ctx.Jsonize("table_type", tableType);
        ctx.Jsonize("pk_field", pkField);
        ctx.Jsonize("operation", operation);
        ctx.Jsonize("table_distribution", tableDist, tableDist);
        ctx.JsonizeAsString("condition", conditionJson, "");
        ctx.JsonizeAsString("modify_field_exprs", modifyFieldExprsJson, "");
        ctx.Jsonize("output_fields", outputFields, outputFields);
        ctx.Jsonize("output_fields_type", outputFieldsType, outputFieldsType);
        if (!tableDist.hashMode.validate()) {
            NAVI_KERNEL_LOG(ERROR, "validate hash mode failed.");
            return false;
        }
    } catch (const autil::legacy::ExceptionBase &e) {
        NAVI_KERNEL_LOG(ERROR, "init failed error:[%s].", e.what());
        return false;
    } catch (...) { return false; }
    stripName();
    return true;
}

void TableModifyInitParam::stripName() {
    KernelUtil::stripName(pkField);
    KernelUtil::stripName(tableDist.hashMode._hashFields);
    KernelUtil::stripName(outputFields);
}

} // end namespace sql
} // end namespace isearch
