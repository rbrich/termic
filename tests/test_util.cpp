// test_util.cc created on 2018-07-30, part of XCI term

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "utility.h"
#include <string>

using namespace xci;
using namespace std::string_literals;


TEST_CASE( "cseq_next_param/explicit", "[utility]" )
{
    auto params = "1;2"s;
    std::string_view params_view = params;
    bool res;
    int p;
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
    std::string_view params_view = params;
    bool res;
    int p;
    constexpr int dfl = -1;

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
