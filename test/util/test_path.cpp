#include <skirmish/util/path.h>
#include "catch.hpp"

using namespace skirmish::util;

TEST_CASE("path") {
    path p("hello/world/foo.bar");
    REQUIRE(p.extension() == ".bar");
    REQUIRE(p.filename() == "foo.bar");
    REQUIRE(p.parent_path() == "hello/world");
}