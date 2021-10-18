// test_util.cc created on 2018-07-30
// This file is part of Termic project <https://github.com/rbrich/termic>
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "utility.h"
#include <string>

using std::string_view;
using namespace xci::term;
using namespace std::string_literals;


TEST_CASE( "cseq_next_param/explicit", "[utility]" )
{
    auto params = "1;2"s;
    string_view params_view = params;
    bool res;
    unsigned int p;
    constexpr int dfl = -1;

    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(res);
    CHECK(p == 1);
    CHECK(params_view == "2");

    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(!res);
    CHECK(p == 2);
}


TEST_CASE( "cseq_next_param/with_defaults", "[utility]" )
{
    auto params = ";1;1234;;"s;
    string_view params_view = params;
    bool res;
    unsigned int p;
    constexpr unsigned int dfl = -1;

    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(res);
    CHECK(p == dfl);
    CHECK(params_view == "1;1234;;");
    
    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(res);
    CHECK(p == 1);
    CHECK(params_view == "1234;;");

    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(res);
    CHECK(p == 1234);
    CHECK(params_view == ";");

    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(res);
    CHECK(p == dfl);
    CHECK(params_view == "");

    p = dfl;
    res = cseq_next_param(params_view, p);
    CHECK(!res);
    CHECK(p == dfl);
}
