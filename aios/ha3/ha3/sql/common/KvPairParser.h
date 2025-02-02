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

#include <functional>
#include <map>
#include <stddef.h>
#include <string>
#include <unordered_map>

#include "autil/Log.h"

namespace isearch {
namespace sql {

class KvPairParser {
public:
    KvPairParser() {}
    ~KvPairParser() {}

private:
    static std::string getNextTerm(const std::string &inputStr, char sep, size_t &start);

public:
    static void parse(const std::string &originalStr,
                      char kvPairSplit,
                      char kvSplit,
                      std::map<std::string, std::string> &kvPair);
    static void parse(const std::string &originalStr,
                      char kvPairSplit,
                      char kvSplit,
                      std::unordered_map<std::string, std::string> &kvPair);
    typedef std::function<void(const std::string &, const std::string &)> SetMap;
    static void
    parse(const std::string &originalStr, char kvPairSplit, char kvSplit, SetMap setMap);

private:
    AUTIL_LOG_DECLARE();
};

} // namespace sql
} // namespace isearch
