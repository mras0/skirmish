#ifndef SKIRMISH_MAT_H
#define SKIRMISH_MAT_H

#include "vec.h"

namespace skirmish {

//
//     ( a00 a01 a02 )   ( A.row[0].x A.row[0].y A.row[0].z )
// A = ( a10 a11 a12 ) = ( A.row[1].x A.row[1].y A.row[1].z )
//     ( a20 a21 a22 )   ( A.row[2].x A.row[2].y A.row[2].z )
//

template<unsigned Rows, unsigned Columns, typename T, typename tag>
struct mat {
    using row_type = vec<Columns, T, tag>;
    row_type row[Rows];

    row_type& operator[](unsigned r) {
        return row[r];
    }

    constexpr const row_type& operator[](unsigned r) const {
        return row[r];
    }

    mat& operator*=(T rhs) {
        for (unsigned r = 0; r < Rows; ++r) {
            (*this)[r] *= rhs;
        }
        return *this;
    }

    bool operator==(const mat& rhs) const {
        for (unsigned r = 0; r < Rows; ++r) {
            if ((*this)[r] != rhs[r]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const mat& rhs) const {
        return !(*this == rhs);
    }
};

template<unsigned Rows, unsigned Columns, typename T, typename tag>
auto operator*(const mat<Rows, Columns, T, tag>& m, const vec<Columns, T, tag>& v)
{
    std::array<T, Columns> res{};

    for (unsigned r = 0; r < Rows; ++r) {
        res[r] = dot(m[r], v);
    }

    return make_vec<tag>(res);
}

template<unsigned R1, unsigned Common, unsigned C2, typename T, typename tag>
auto operator*(const mat<R1, Common, T, tag>& lhs, const mat<Common, C2, T, tag>& rhs)
{
    mat<R1, C2, T, tag> res{};

    for (unsigned r = 0; r < R1; ++ r) {
        for (unsigned c = 0; c < C2; ++c) {
            for (unsigned i = 0; i < Common; ++i) {
                res[r][c] += lhs[r][i] * rhs[i][c];
            }
        }
    }

    return res;
}

template<unsigned Rows, unsigned Columns, typename T, typename tag>
auto operator*(const mat<Rows, Columns, T, tag>& m, T scale)
{
    auto res = m;
    return res *= scale;
}

template<unsigned Rows, unsigned Columns, typename T, typename tag>
auto operator*(T scale, const mat<Rows, Columns, T, tag>& m)
{
    return m * scale;
}

} // namespace skirmish

#endif