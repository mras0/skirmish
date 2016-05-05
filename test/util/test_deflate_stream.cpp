#include <skirmish/util/deflate_stream.h>
#include "catch.hpp"

using namespace skirmish::util;

TEST_CASE("deflate test") {
    const uint8_t deflate_input[13] = {0xf3, 0xc9, 0xcc, 0x4b, 0x55, 0x30, 0xe4, 0xf2, 0x01, 0x51, 0x46, 0x5c, 0x00};
    in_mem_stream compressed_stream{deflate_input};
    in_deflate_stream uncompressed_stream{compressed_stream};
    char output[14];
    uncompressed_stream.read(output, sizeof(output));
    REQUIRE(std::string(output, output+sizeof(output)) == "Line 1\nLine 2\n");
}