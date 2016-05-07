#include <skirmish/math/vec.h>
#include <skirmish/math/mat.h>
#include "matvecio.h"
#include <type_traits>
#include "catch.hpp"

using namespace skirmish;

struct tag1;
using v2f = vec<2, float, tag1>;
using v3f = vec<3, float, tag1>;
using v4f = vec<3, float, tag1>;

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

TEST_CASE("vector helpers") {
    REQUIRE((detail::make_diag_row<3, float, tag1>(0, 8.0f)) == (v3f{8.0f,0,0}));
    REQUIRE((detail::make_diag_row<3, float, tag1>(1, -3.0f)) == (v3f{0,-3.0f,0}));
    REQUIRE((detail::make_diag_row<3, float, tag1>(2, 2.1f)) == (v3f{0,0,2.1f}));
    REQUIRE((detail::make_diag_row<3, float, tag1>(3, 3.0f)) == (v3f{0,0,0}));
}

TEST_CASE("mat3*f") {
    using mat33 = mat<3, 3, float, tag1>;
    using mat32 = mat<3, 2, float, tag1>;
    mat33 m{1, 2, 3, 
            4, 5, 6,
            7, 8, 9};
    mat32 n{1, 2,
            3, 4,
            5, 6};
    static_assert(std::is_same_v<mat33::row_type, v3f>, "");
    static_assert(std::is_same_v<decltype(m.col(0)), v3f>, "");
    static_assert(std::is_same_v<mat32::row_type, v2f>, "");
    static_assert(std::is_same_v<decltype(n.col(0)), v3f>, "");

    SECTION("init") {
        REQUIRE(m[0].x() == 1);
        REQUIRE(m[0].y() == 2);
        REQUIRE(m[0].z() == 3);
        REQUIRE(m[1].x() == 4);
        REQUIRE(m[1].y() == 5);
        REQUIRE(m[1].z() == 6);
        REQUIRE(m[2].x() == 7);
        REQUIRE(m[2].y() == 8);
        REQUIRE(m[2].z() == 9);
        REQUIRE(m.col(0) == (v3f{1,4,7}));
        REQUIRE(m.col(1) == (v3f{2,5,8}));
        REQUIRE(m.col(2) == (v3f{3,6,9}));
        REQUIRE(n.row(0) == (v2f{1,2}));
        REQUIRE(n.col(0) == (v3f{1,3,5}));
        REQUIRE(n.col(1) == (v3f{2,4,6}));
        REQUIRE(mat32::zero()     == (mat32{0,0,0,0,0,0}));
        REQUIRE(mat32::identity() == (mat32{1,0,0,1,0,0}));
        REQUIRE(mat33::identity() == (mat33{1,0,0,0,1,0,0,0,1}));
    }
    SECTION("equality") {
        REQUIRE(m == m);
        REQUIRE(n == n);
        REQUIRE(n != (mat32{2,1,3,4,5,6}));
    }
    SECTION("mul vec3") {
        const v3f v{-2, 4.5f, 12};
        const v3f expected{-2*1+4.5f*2+12*3, -2*4+4.5f*5+12*6, -2*7+4.5f*8+12*9};
        REQUIRE(m * v == expected);
    }
    SECTION("scale") {
        REQUIRE((m*-2.0f)[1][1] == -10.0f);
        REQUIRE((-2.0f*m)[1][1] == -10.0f);
    }

    SECTION("mul 33 32") {
        REQUIRE(m*n == (mat32{
            1*1+2*3+3*5, 1*2+2*4+3*6,
            4*1+5*3+6*5, 4*2+5*4+6*6,
            7*1+8*3+9*5, 7*2+8*4+9*6,
        }));
    }
}
