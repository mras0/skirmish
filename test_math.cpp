#include "vec.h"
#include <type_traits>
#include <ostream>

template<unsigned size, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::vec<size, T, tag>& v) {
    static_assert(size == 3, "");
    return os << "{ " << v.x << ", " << v.y << ", " << v.z << " }";
}

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace skirmish;

struct tag1;
using vec3ft1 = vec3f<tag1>;

static_assert(std::is_pod<vec3ft1>::value, "vec3 must be POD!");

TEST_CASE("vec3 construction") {
    REQUIRE((vec3ft1{1,2,3}).x == 1);
    REQUIRE((vec3ft1{1,2,3}).y == 2);
    REQUIRE((vec3ft1{1,2,3}).z == 3);
}

TEST_CASE("vec3 boolean logic") {
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

TEST_CASE("vec3 multiplication") {
    vec3ft1 a{10, 20, 100};
    SECTION("negative") {
        REQUIRE(a * -2.0f == (vec3ft1{-20, -40, -200}));
        REQUIRE(-2.0f * a == (vec3ft1{-20, -40, -200}));
        REQUIRE((a *= -2) == (vec3ft1{-20, -40, -200}));
    }
    SECTION("zero") {
        REQUIRE((a *= 0) == (vec3ft1{0, 0, 0}));
    }
    SECTION("positive") {
        REQUIRE((a *= 1.1f) == (vec3ft1{11, 22, 110}));
    }
}

TEST_CASE("vec3 addition") {
    vec3ft1 a{1, 2, 3};
    vec3ft1 b{-3, 2.5f, 10};
    const vec3ft1 sum{-2, 4.5f, 13};
    REQUIRE(a + b == sum);
    REQUIRE(b + a == sum);
    SECTION("a+=b") {
        REQUIRE((a += b) == sum);
    }
    SECTION("b+=a") {
        REQUIRE((b += a) == sum);
    }
}

TEST_CASE("vec3 negation") {
    const vec3ft1 a{7, 4, -1};
    const vec3ft1 na{-7, -4, 1};
    REQUIRE(-a == na);
    REQUIRE(-na == a);
    REQUIRE(-(-a) == a);
}

TEST_CASE("vec3 subtraction") {
    vec3ft1 a{1, 2, 3};
    vec3ft1 b{-3, 2.5f, 10};
    const vec3ft1 diff_ab{4, -0.5f, -7};
    const vec3ft1 diff_ba{-4, 0.5f, 7};
    REQUIRE(a - b == diff_ab);
    REQUIRE(b - a == diff_ba);
    SECTION("a-=b") {
        REQUIRE((a -= b) == diff_ab);
    }
    SECTION("b-=a") {
        REQUIRE((b -= a) == diff_ba);
    }
}