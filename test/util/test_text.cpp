#include <skirmish/util/text.h>
#include <skirmish/util/stream.h>
#include "catch.hpp"

using namespace skirmish::util;

TEST_CASE("trim") {
    REQUIRE(trim("") == "");
    REQUIRE(trim("\r\n") == "");
    REQUIRE(trim("bl ah \tblah") == "bl ah \tblah");
    REQUIRE(trim("  \n\r\tbl ah \tblah") == "bl ah \tblah");
    REQUIRE(trim("bl ah \tblah  \n\t") == "bl ah \tblah");
    REQUIRE(trim("    bl ah \tblah  \n\t") == "bl ah \tblah");
}

TEST_CASE("read_line") {
    const char data[] = "Line One\nLine Two\r\n\n123";
    in_mem_stream s{make_array_view(data, data+sizeof(data)-1)};

    std::string line;
    REQUIRE(read_line(s, line));
    REQUIRE(s.error() == std::error_code());
    REQUIRE(line == "Line One");

    REQUIRE(read_line(s, line));
    REQUIRE(s.error() == std::error_code());
    REQUIRE(line == "Line Two\r");

    REQUIRE(read_line(s, line));
    REQUIRE(s.error() == std::error_code());
    REQUIRE(line == "");

    REQUIRE(!read_line(s, line));   
    REQUIRE(line == "123");
    REQUIRE(s.error() != std::error_code());

    REQUIRE(!read_line(s, line));   
    REQUIRE(line == "");
    REQUIRE(s.error() != std::error_code());
}