#include <skirmish/util/stream.h>
#include "catch.hpp"
#include <vector>
#include <iterator>

using namespace skirmish::util;
using bytevec = std::vector<std::uint8_t>;

TEST_CASE("zero stream") {
    zero_stream s;
    bytevec v;
    for (int i = 0; i < 10000; ++i) {
        v.push_back(s.get());
    }
    REQUIRE(v == bytevec(10000));
    REQUIRE(s.error() == std::error_code());
}

TEST_CASE("mem stream") {
    const char buffer[] = "hello world";
    mem_stream s(buffer);
    REQUIRE(s.get() == 'h');
    REQUIRE(s.get() == 'e');
    REQUIRE(s.get() == 'l');
    REQUIRE(s.get() == 'l');
    REQUIRE(s.get() == 'o');
    REQUIRE(s.get() == ' ');
    REQUIRE(s.get() == 'w');
    REQUIRE(s.get() == 'o');
    REQUIRE(s.get() == 'r');
    REQUIRE(s.get() == 'l');
    REQUIRE(s.get() == 'd');
    REQUIRE(s.get() == 0);
    REQUIRE(s.error() == std::error_code());
    REQUIRE(s.get() == 0);
    REQUIRE(s.error() != std::error_code());
}
