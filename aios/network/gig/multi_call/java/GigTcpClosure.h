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
#ifndef ISEARCH_MULTI_CALL_GIGTCPCLOSURE_H
#define ISEARCH_MULTI_CALL_GIGTCPCLOSURE_H

#include "aios/network/gig/multi_call/java/GigJavaClosure.h"

namespace multi_call {

class GigTcpClosure : public GigJavaClosure {
public:
    GigTcpClosure(JavaCallback callback, long callbackId,
                  const GigRequestGeneratorPtr &generator);
    ~GigTcpClosure();

private:
    GigTcpClosure(const GigTcpClosure &);
    GigTcpClosure &operator=(const GigTcpClosure &);

public:
    virtual bool extractResponse(ResponsePtr response,
                                 GigResponseHeader *responseHeader,
                                 const char *&body, size_t &bodySize) override;

private:
    AUTIL_LOG_DECLARE();
};

MULTI_CALL_TYPEDEF_PTR(GigTcpClosure);

} // namespace multi_call

#endif // ISEARCH_MULTI_CALL_GIGTCPCLOSURE_H
