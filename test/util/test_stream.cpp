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

TEST_CASE("input file stream") {
    in_file_stream invalid_file_name{"this_file_does_not_exist"};
    REQUIRE(invalid_file_name.error() != std::error_code());

    const auto fname = (std::string{TEST_DATA_DIR} + "/" + "test.txt");
    in_file_stream test_txt{fname.c_str()};
    REQUIRE(test_txt.error() == std::error_code());
    REQUIRE(test_txt.get() == 'L');
    REQUIRE(test_txt.get() == 'i');
    REQUIRE(test_txt.get() == 'n');
    REQUIRE(test_txt.get() == 'e');
    REQUIRE(test_txt.get() == ' ');
    REQUIRE(test_txt.get() == '1');
    REQUIRE(test_txt.get() == '\n');
    REQUIRE(test_txt.get() == 'L');
    REQUIRE(test_txt.get() == 'i');
    REQUIRE(test_txt.get() == 'n');
    REQUIRE(test_txt.get() == 'e');
    REQUIRE(test_txt.get() == ' ');
    REQUIRE(test_txt.get() == '2');
    REQUIRE(test_txt.get() == '\n');
    REQUIRE(test_txt.error() == std::error_code());
    REQUIRE(test_txt.get() == 0);
    REQUIRE(test_txt.error() != std::error_code());
}

TEST_CASE("little endian") {
    const uint8_t u16_fede[] = { 0xde, 0xfe };
    REQUIRE(mem_stream(u16_fede).get_u16_le() == 0xfede);
    const uint8_t u32_fede0abe[] = { 0xbe, 0x0a, 0xde, 0xfe };
    REQUIRE(mem_stream(u32_fede0abe).get_u32_le() == 0xfede0abe);
}