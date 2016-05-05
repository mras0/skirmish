#include <skirmish/util/array_view.h>
#include "catch.hpp"

using namespace skirmish::util;

template<typename T>
bool operator==(const array_view<T>& l, const array_view<T>& r) {
    return l.data() == r.data() && l.size() == r.size();
}

template<typename T>
bool operator!=(const array_view<T>& l, const array_view<T>& r) {
    return !(l == r);
}

TEST_CASE("array_view") {
    char arr[4] = { 'b', 'l', 'a', 'h' };
    auto av = make_array_view(arr);
    static_assert(std::is_same_v<decltype(av), array_view<char>>, "");
    REQUIRE(av.data() == &arr[0]);
    REQUIRE(av.size() == 4);
    REQUIRE(av.begin() == &arr[0]);
    REQUIRE(av.end() == &arr[sizeof(arr)]);
    REQUIRE(av[0] == 'b');
    REQUIRE(av[1] == 'l');
    REQUIRE(av[2] == 'a');
    REQUIRE(av[3] == 'h');
    REQUIRE(make_array_view(&arr[0], 4) == av);
    REQUIRE(make_array_view(&arr[0], &arr[4]) == av);
}