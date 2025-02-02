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
#include "aios/network/gig/multi_call/java/GigArpcGenerator.h"

using namespace std;

namespace multi_call {
AUTIL_LOG_SETUP(multi_call, GigArpcGenerator);


RequestPtr GigArpcGenerator::generateRequest(const std::string &bodyStr,
        const GigRequestPlan &requestPlan) {
    GigArpcRequestPtr request(new GigArpcRequest(getProtobufArena()));
    if (requestPlan.has_timeout()) {
        request->setTimeout(requestPlan.timeout());
    }
    if (!requestPlan.has_service_id() || !requestPlan.has_method_id()) {
        AUTIL_LOG(ERROR, "service id and method id required");
        return RequestPtr();
    }
    request->setServiceId(requestPlan.service_id());
    request->setMethodId(requestPlan.method_id());

    request->setPacket(bodyStr);
    return request;
}

} // namespace multi_call
