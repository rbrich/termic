// utility.cpp created on 2018-07-30
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "utility.h"
#include <xci/core/log.h>
#include <cstdlib>

namespace xci::term {

using namespace xci::core;


bool cseq_next_param(std::string_view& params, unsigned& p)
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


void cseq_parse_params(const char *name, std::string_view &params, unsigned& p1)
{
    if (cseq_next_param(params, p1))
        log::warning("Excess params for {} ignored: {}", name, params);
}


void cseq_parse_params(const char *name, std::string_view &params, unsigned& p1, unsigned& p2)
{
    if (cseq_next_param(params, p1) && cseq_next_param(params, p2))
        log::warning("Excess params for {} ignored: {}", name, params);
}


} // namespace xci::term
