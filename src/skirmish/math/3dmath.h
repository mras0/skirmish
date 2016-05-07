#ifndef SKIRMISH_MATH_3DMATH_H
#define SKIRMISH_MATH_3DMATH_H

#include "mat.h"
#include <math.h>

namespace skirmish {

template<unsigned Rows, unsigned Columns, typename T, typename tag>
struct matrix_factory {
    static_assert(Rows >= 3 && Columns >= 3, "Matrix is too small");
    using mat_type = mat<Rows, Columns, T, tag>;

    static mat_type rotation_x(T theta) {
        auto res = mat<Rows, Columns, T, tag>::identity();
        const auto cos_theta = T(cos(theta));
        const auto sin_theta = T(sin(theta));
        res[1][1] = cos_theta; res[1][2] = -sin_theta;
        res[2][1] = sin_theta; res[2][2] =  cos_theta;
        return res;
    }

    static mat_type rotation_y(T theta) {
        auto res = mat<Rows, Columns, T, tag>::identity();
        const auto cos_theta = T(cos(theta));
        const auto sin_theta = T(sin(theta));
        res[0][0] =  cos_theta; res[0][2] = sin_theta;
        res[2][0] = -sin_theta; res[2][2] = cos_theta;
        return res;
    }

    static mat_type rotation_z(T theta) {
        auto res = mat<Rows, Columns, T, tag>::identity();
        const auto cos_theta = T(cos(theta));
        const auto sin_theta = T(sin(theta));
        res[0][0] = cos_theta; res[0][1] = -sin_theta;
        res[1][0] = sin_theta; res[1][1] =  cos_theta;
        return res;
    }

    static mat_type translation(const vec<Rows - 1, T, tag>& v) {
        static_assert(Columns > 3, "Matrix too small");
        auto res = mat<Rows, Columns, T, tag>::identity();
        for (unsigned r = 0; r < Rows - 1; ++r) {
            res[r][3] = v[r];
        }
        return res;
    }
};

} // namespace skirmish

#endif
