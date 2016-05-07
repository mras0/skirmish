#include "catch.hpp"
#include <skirmish/math/3dmath.h>
#include <skirmish/math/constants.h>
#include "matvecio.h"

using namespace skirmish;
struct my_tag;
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