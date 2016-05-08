#ifndef SKIRMISH_MAT_H
#define SKIRMISH_MAT_H

#include "vec.h"
#include <utility> // make_index_sequence

namespace skirmish {

//                           col0       col1       col2
//     ( a00 a01 a02 )   ( A.row[0].x A.row[0].y A.row[0].z )
// A = ( a10 a11 a12 ) = ( A.row[1].x A.row[1].y A.row[1].z )
//     ( a20 a21 a22 )   ( A.row[2].x A.row[2].y A.row[2].z )
//

template<unsigned Rows, unsigned Columns, typename T, typename tag>
struct matrix_factory;

template<unsigned Rows, unsigned Columns, typename T, typename tag>
struct mat {
    using factory      = matrix_factory<Rows, Columns, T, tag>;
    using row_type     = vec<Columns, T, tag>;
    row_type rows[Rows];

    row_type& operator[](unsigned r) {
        return rows[r];
    }

    constexpr const row_type& operator[](unsigned r) const {
        return rows[r];
    }

    row_type& row(unsigned r) {
        return rows[r];
    }

    constexpr const row_type& row(unsigned r) const {
        return rows[r];
    }

    template<std::size_t... I>
    constexpr vec<sizeof...(I), T, tag> select_from_column(unsigned column, std::index_sequence<I...>) const {
        return {rows[I][column]...};
    }

    constexpr vec<Rows, T, tag> col(unsigned column) const {
        return select_from_column(column, std::make_index_sequence<Rows>());
    }

    mat& operator*=(T rhs) {
        for (unsigned r = 0; r < Rows; ++r) {
            (*this)[r] *= rhs;
        }
        return *this;
    }

    mat& operator+=(const mat& rhs) {
        for (unsigned r = 0; r < Rows; ++r) {
            (*this)[r] += rhs[r];
        }
        return *this;
    }

    mat& operator-=(const mat& rhs) {
        for (unsigned r = 0; r < Rows; ++r) {
            (*this)[r] -= rhs[r];
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
    
    constexpr static mat zero() {
        return {};
    }

    constexpr static mat identity();
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
    mat<R1, C2, T, tag> res;

    for (unsigned r = 0; r < R1; ++ r) {
        for (unsigned c = 0; c < C2; ++c) {
            res[r][c] = dot(lhs.row(r), rhs.col(c));
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

template<unsigned Rows, unsigned Columns, typename T, typename tag>
auto operator+(const mat<Rows, Columns, T, tag>& l, const mat<Rows, Columns, T, tag>& r)
{
    auto res = l;
    return res += r;
}

template<unsigned Rows, unsigned Columns, typename T, typename tag>
auto operator-(const mat<Rows, Columns, T, tag>& l, const mat<Rows, Columns, T, tag>& r)
{
    auto res = l;
    return res -= r;
}

namespace detail {

template<unsigned Size, typename T, typename tag, std::size_t... I>
constexpr vec<Size, T, tag> make_diag_row_impl(size_t index, const T& value, std::index_sequence<I...>) {
    return { (I == index ? value : static_cast<T>(0))... };
}

template<unsigned Size, typename T, typename tag>
constexpr vec<Size, T, tag> make_diag_row(size_t index, const T& value) {
    return make_diag_row_impl<Size, T, tag>(index, value, std::make_index_sequence<Size>());
}

template<unsigned Columns, typename T, typename tag, std::size_t... I>
constexpr mat<sizeof...(I), Columns, T, tag> make_identity_impl(std::index_sequence<I...>)
{
    return { make_diag_row<Columns, T, tag>(I, static_cast<T>(1))... };
}

} // namespace detail

template<unsigned Rows, unsigned Columns, typename T, typename tag>
constexpr mat<Rows, Columns, T, tag> mat<Rows, Columns, T, tag>::identity()
{
    return detail::make_identity_impl<Columns, T, tag>(std::make_index_sequence<Rows>());
}

template<unsigned Rows, unsigned Columns, typename T, typename tag>
mat<Columns, Rows, T, tag> transposed(const mat<Rows, Columns, T, tag>& m)
{
    // TODO: optimize
    mat<Columns, Rows, T, tag> res;
    for (unsigned c = 0; c < Columns; ++c) {
        for (unsigned r = 0; r < Rows; ++r) {
            res[c][r] = m[r][c];
        }
    }
    return res;
}

} // namespace skirmish

#endif