// utility.h created on 2018-07-30
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCITERM_UTILITY_H
#define XCITERM_UTILITY_H

#include <string_view>

namespace xci::term {

/// Parses parameter string of control sequence
/// according to ECMA-48 standard.
/// \param[in,out]  params  A string view with the params.
///                         The parsed prefix will be removed.
/// \param[out]     p       The parsed parameter will be put here.
///                         If no digits were parsed, the content won't be modified.
/// \return         True if there are more parameters to be parsed.
///                 False if this was the last one.
bool cseq_next_param(std::string_view& params, unsigned& p);

void cseq_parse_params(const char* name, std::string_view& params, unsigned& p1);
void cseq_parse_params(const char* name, std::string_view& params, unsigned& p1, unsigned& p2);

} // namespace xci::term

#endif // XCITERM_UTILITY_H
