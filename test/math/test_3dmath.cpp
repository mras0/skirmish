#include "catch.hpp"
#include <skirmish/math/3dmath.h>
#include <skirmish/math/constants.h>
#include "matvecio.h"

using namespace skirmish;
struct my_tag;
using v3  = vec<3, float, my_tag>;
using m33 = mat<3, 3, float, my_tag>;
using m44 = mat<4, 4, float, my_tag>;

using factory3 = matrix_factory<3, 3, float, my_tag>;
using factory4 = matrix_factory<4, 4, float, my_tag>;

template<unsigned Rows, unsigned Columns, typename T, typename tag>
auto trunc_small_values(const mat<Rows, Columns, T, tag>& in) {
    mat<Rows, Columns, T, tag> m = in;
    for (unsigned r = 0; r < Rows; ++r) {
        for (unsigned c = 0; c < Columns; ++c) {
            if (fabs(m[r][c]) < 1e-7) m[r][c] = 0;
        }
    }
    return m;
}

TEST_CASE("Rotation matricies") {
    auto rx90 = trunc_small_values(factory3::rotation_x(pi_f / 2.0f));
    REQUIRE(rx90 == (m33{ 1,  0,  0, 
                          0,  0, -1,
                          0,  1,  0}));

    auto ry90 = trunc_small_values(factory3::rotation_y(pi_f / 2.0f));
    REQUIRE(ry90 == (m33{ 0,  0,  1, 
                          0,  1,  0,
                         -1,  0,  0}));

    auto rz90 = trunc_small_values(factory3::rotation_z(pi_f / 2.0f));
    REQUIRE(rz90 == (m33{ 0, -1,  0, 
                          1,  0,  0,
                          0,  0,  1}));

    auto rz45 = trunc_small_values(factory3::rotation_z(pi_f / 4.0f));
    constexpr auto s2 = sqrt2_f/2.0f;
    REQUIRE(rz45 == (m33{s2, -s2,  0, 
                         s2,  s2,  0,
                          0,   0,  1}));

    auto rz90_4x4 = trunc_small_values(factory4::rotation_z(pi_f /2.0f));
    REQUIRE(rz90_4x4 == (m44{0,-1,0,0, 1,0,0,0, 0,0,1,0, 0,0,0,1}));
}

TEST_CASE("Translation matrices") {
    const auto t = factory4::translation(vec<3, float, my_tag>{1,2,3});
    REQUIRE(t == (m44{
        1, 0, 0, 1,
        0, 1, 0, 2,
        0, 0, 1, 3,
        0, 0, 0, 1,
    }));
}

TEST_CASE("Vector cross product") {
    REQUIRE(cross((v3{1, 0, 0}), (v3{0, 1, 0})) == (v3{0, 0, 1}));
    REQUIRE(cross((v3{0, 3, 0}), (v3{0, 0, 4})) == (v3{12, 0, 0}));
    REQUIRE(cross((v3{1, 2, 3}), (v3{4, 5, 6})) == (v3{-3, 6, -3}));
}

TEST_CASE("LookAtLH") {
    const m44 res1{
        0.70710677f, -0.40824828f, -0.57735026f,  0.00000000f,
        -0.70710677f, -0.40824828f, -0.57735026f,  0.00000000f,
        0.00000000f,  0.81649655f, -0.57735026f,  0.00000000f,
        0.00000000f,  0.00000000f,  3.46410155f,  1.00000000f,
    };
    REQUIRE(m44::factory::look_at_lh((v3{2.0f, 2.0f, 2.0f}), (v3{0.0f, 0.0f, 0.0f}), (v3{0.0f, 0.0f, 1.0f})) == res1);

    const m44 res2{
        -0.92847675f, -0.29436204f, -0.22645542f,  0.00000000f,
        -0.37139070f,  0.73590505f,  0.56613857f,  0.00000000f,
         0.00000000f,  0.60974991f, -0.79259396f,  0.00000000f,
         1.29986739f,  0.77795696f,  6.90689039f,  1.00000000f,
    };
    REQUIRE(m44::factory::look_at_lh((v3{3.0f, -4.0f, 5.0f}), (v3{1.0f, 1.0f, -2.0f}), (v3{0.0f, 0.0f, 1.0f})) == res2);
}

TEST_CASE("PerspectiveFovLH") {
    m44 expected{
         0.75000000f,  0.00000000f,  0.00000000f,  0.00000000f,
         0.00000000f,  1.00000000f,  0.00000000f,  0.00000000f,
         0.00000000f,  0.00000000f,  1.00010002f,  1.00000000f,
         0.00000000f,  0.00000000f, -0.01000100f,  0.00000000f,
    };
    REQUIRE(m44::factory::perspective_fov_lh(pi_f / 2.0f, 640.0f/480.0f, 0.01f, 100.0f) == expected);
}