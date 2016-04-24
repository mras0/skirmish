#include "vec.h"
#include "mat.h"
#include <type_traits>
#include <ostream>

template<unsigned size, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::vec<size, T, tag>& v) {
    static_assert(size == 3, "");
    return os << "{ " << v.x() << ", " << v.y() << ", " << v.z() << " }";
}

template<unsigned Rows, unsigned Columns, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::mat<Rows, Columns, T, tag>& m) {
    for (unsigned r = 0; r < Rows; ++r) {
        os << m[r] << '\n';
    }
    return os;
}

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace skirmish;

struct tag1;
using v3f = vec<3, float, tag1>;

static_assert(std::is_pod<v3f>::value, "vec3 must be POD!");

TEST_CASE("vec3 construction") {
    REQUIRE((v3f{1,2,3}).x() == 1);
    REQUIRE((v3f{1,2,3}).y() == 2);
    REQUIRE((v3f{1,2,3}).z() == 3);
    REQUIRE((v3f{1,2,3})[0] == 1);
    REQUIRE((v3f{1,2,3})[1] == 2);
    REQUIRE((v3f{1,2,3})[2] == 3);
    REQUIRE(v3f() == (v3f{0,0,0}));
    REQUIRE(make_vec<tag1>(std::array<float, 3>{1,2,3}) == (v3f{1,2,3}));
}

TEST_CASE("vec3 boolean logic") {
    const v3f a{1,2,3};
    const v3f b{1.001f,2,3};
    const v3f c{1,2.1f,3};
    const v3f d{1,2,2};
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
    v3f a{10, 20, 100};
    SECTION("negative") {
        REQUIRE(a * -2.0f == (v3f{-20, -40, -200}));
        REQUIRE(-2.0f * a == (v3f{-20, -40, -200}));
        REQUIRE((a *= -2) == (v3f{-20, -40, -200}));
    }
    SECTION("zero") {
        REQUIRE((a *= 0) == (v3f{0, 0, 0}));
    }
    SECTION("positive") {
        REQUIRE((a *= 1.1f) == (v3f{11, 22, 110}));
    }
}

TEST_CASE("vec3 addition") {
    v3f a{1, 2, 3};
    v3f b{-3, 2.5f, 10};
    const v3f sum{-2, 4.5f, 13};
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
    const v3f a{7, 4, -1};
    const v3f na{-7, -4, 1};
    REQUIRE(-a == na);
    REQUIRE(-na == a);
    REQUIRE(-(-a) == a);
}

TEST_CASE("vec3 subtraction") {
    v3f a{1, 2, 3};
    v3f b{-3, 2.5f, 10};
    const v3f diff_ab{4, -0.5f, -7};
    const v3f diff_ba{-4, 0.5f, 7};
    REQUIRE(a - b == diff_ab);
    REQUIRE(b - a == diff_ba);
    SECTION("a-=b") {
        REQUIRE((a -= b) == diff_ab);
    }
    SECTION("b-=a") {
        REQUIRE((b -= a) == diff_ba);
    }
}

TEST_CASE("vec3 dot") {
    v3f a{1, 2, 3};
    v3f b{4, 5, 6};
    REQUIRE(dot(a, b) == (4+10+18));
}

TEST_CASE("mat33") {
    using mat33ft1 = mat<3, 3, float, tag1>;
    mat33ft1 a{1, 2, 3, 4, 5, 6, 7, 8, 9};
    SECTION("init") {
        REQUIRE(a[0].x() == 1);
        REQUIRE(a[0].y() == 2);
        REQUIRE(a[0].z() == 3);
        REQUIRE(a[1].x() == 4);
        REQUIRE(a[1].y() == 5);
        REQUIRE(a[1].z() == 6);
        REQUIRE(a[2].x() == 7);
        REQUIRE(a[2].y() == 8);
        REQUIRE(a[2].z() == 9);
    }
    SECTION("mul vec3") {
        const v3f v{-2, 4.5f, 12};
        const v3f expected{-2*1+4.5f*2+12*3, -2*4+4.5f*5+12*6, -2*7+4.5f*8+12*9};
        REQUIRE(a * v == expected);
    }
    SECTION("scale") {
        REQUIRE((a*-2.0f)[1][1] == -10.0f);
        REQUIRE((-2.0f*a)[1][1] == -10.0f);
    }
}