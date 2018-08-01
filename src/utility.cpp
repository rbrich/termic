// utility.cpp created on 2018-07-30, part of XCI term
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "utility.h"
#include <cstdlib>

namespace xci {

bool cseq_next_param(std::string_view& params, int& p)
{
    bool p_touched = false;
    while (!params.empty()) {
        char c = params[0];
        if (isdigit(c)) {
            if (!p_touched) {
                p = 0;
                p_touched = true;
            }
            p = p * 10 + (c - '0');
        } else if (c == ';') {
            params.remove_prefix(1);
            return true;
        }
        params.remove_prefix(1);
    }
    return false;
}

} // namespace xci