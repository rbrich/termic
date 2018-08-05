// utility.h created on 2018-07-30, part of XCI term
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

#ifndef XCITERM_UTILITY_H
#define XCITERM_UTILITY_H

#include <xci/compat/string_view.h>

namespace xci {

/// Parses parameter string of control sequence
/// according to ECMA-48 standard.
/// \param params [INOUT] A string view with the params.
///               The parsed prefix will be removed.
/// \param p      [OUT] The parsed parameter will be put here.
///               If no digits were parsed, the content won't be modified.
/// \return       True if there are more parameters to be parsed.
///               False if this was the last one.
bool cseq_next_param(std::string_view& params, int& p);

void cseq_parse_params(const char* name, std::string_view& params, int& p1);
void cseq_parse_params(const char* name, std::string_view& params, int& p1, int& p2);

} // namespace xci

#endif // XCITERM_UTILITY_H
