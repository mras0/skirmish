#ifndef SKIRMISH_MATH_3DMATH_H
#define SKIRMISH_MATH_3DMATH_H

#include "mat.h"
#include <math.h>

namespace skirmish {

template<typename T, typename tag>
vec<3, T, tag> cross(const vec<3, T, tag>& u, const vec<3, T, tag>& v) {
    return {
        u[1] * v[2] - u[2] * v[1],
        u[2] * v[0] - u[0] * v[2],
        u[0] * v[1] - u[1] * v[0]
    };
}

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

    template<typename vector_tag>
    static mat_type look_at_lh(const vec<3, T, vector_tag>& eye, const vec<3, T, vector_tag>& at, const vec<3, T, vector_tag>& up) {
        static_assert(Columns > 3 && Rows > 3, "Matrix too small");
        const auto zaxis = normalized(at - eye);
        const auto xaxis = normalized(cross(up, zaxis));
        const auto yaxis = cross(zaxis, xaxis);
        auto res = mat<Rows, Columns, T, tag>::identity();
        res[0][0] = xaxis[0]; res[0][1] = yaxis[0]; res[0][2] = zaxis[0];
        res[1][0] = xaxis[1]; res[1][1] = yaxis[1]; res[1][2] = zaxis[1];
        res[2][0] = xaxis[2]; res[2][1] = yaxis[2]; res[2][2] = zaxis[2];
        res[3][0] = dot(xaxis, -eye); res[3][1] = dot(yaxis, -eye); res[3][2] = dot(zaxis, -eye); 
        return res;
    }

    static mat_type perspective_fov_lh(T fov_angle_y, T aspect_ratio, T near_z, T far_z) {
        static_assert(Columns > 3 && Rows > 3, "Matrix too small");
        const T h = 1 / tan(fov_angle_y/2);
        const T w = h / aspect_ratio;
        auto res = mat<Rows, Columns, T, tag>::identity();
        res[0][0] = w;
                       res[1][1] = h;
                       res[2][2] = far_z/(far_z-near_z);          res[2][3] = 1.0f;
                       res[3][2] = -near_z*far_z/(far_z-near_z);  res[3][3] = 0.0f;
        return res;
    }
};

} // namespace skirmish

#endif
