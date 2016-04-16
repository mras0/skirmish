#include "vec.h"
#include <type_traits>
#include <ostream>

template<typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::vec3<T, tag>& v) {
    return os << "{ " << v.x << ", " << v.y << ", " << v.z << " }";
}

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace skirmish;

struct tag1;
using vec3ft1 = vec3f<tag1>;

static_assert(std::is_pod<vec3ft1>::value, "vec3 must be POD!");

TEST_CASE("Construction works") {
    REQUIRE((vec3ft1{1,2,3}).x == 1);
    REQUIRE((vec3ft1{1,2,3}).y == 2);
    REQUIRE((vec3ft1{1,2,3}).z == 3);
}

TEST_CASE("Boolean logic") {
    const vec3ft1 a{1,2,3};
    const vec3ft1 b{1.001f,2,3};
    const vec3ft1 c{1,2.1f,3};
    const vec3ft1 d{1,2,2};
    REQUIRE(a == a);
    REQUIRE(b == b);
    REQUIRE(c == c);
    REQUIRE(d == d);
    REQUIRE(a != b);
    REQUIRE(a != c);
    REQUIRE(a != d);
    REQUIRE(b != a);
    REQUIRE(b != c);
    REQUIRE(b != d);
    REQUIRE(c != d);
    REQUIRE(d != c);
}

TEST_CASE("Multiplication") {
    vec3ft1 a{10, 20, 100};
    SECTION("negative") {
        a *= -2;
        REQUIRE(a == (vec3ft1{-20, -40, -200}));
    }
    SECTION("zero") {
        a *= 0;
        REQUIRE(a == (vec3ft1{0, 0, 0}));
    }
    SECTION("positive") {
        a *= 1.1f;
        REQUIRE(a == (vec3ft1{11, 22, 110}));
    }
}