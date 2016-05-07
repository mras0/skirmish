#include <skirmish/util/file_system.h>
#include "catch.hpp"

using namespace skirmish::util;

TEST_CASE("native fs") {
    native_file_system fs{TEST_DATA_DIR};
    auto file_list = fs.file_list();
    std::sort(file_list.begin(), file_list.end());
    REQUIRE(file_list == (std::vector<path>{ "empty.zip", "test.txt", "test.zip", "test_data.zip"}));

    auto fptr = fs.open("test.txt");
    REQUIRE(fptr->error() == std::error_code());
    char buf[14];
    fptr->read(buf, sizeof(buf));
    REQUIRE(std::string(buf, buf+sizeof(buf)) == "Line 1\nLine 2\n");
    REQUIRE(fptr->error() == std::error_code());
}