#ifndef SKIRMISH_VEC_H
#define SKIRMISH_VEC_H

#include <array>
#include <utility> // make_index_sequence
#include <math.h>

namespace skirmish {

template<unsigned Size, typename T, typename tag>
struct vec {
    T v[Size];

    T& operator[](size_t index) {
        return v[index];
    }

    constexpr T operator[](size_t index) const {
        return v[index];
    }

#define MAKE_VEC_ACCESSOR(a_name, a_idx) T& a_name () { return v[a_idx]; } constexpr T a_name () const { return v[a_idx]; }
MAKE_VEC_ACCESSOR(x, 0)
MAKE_VEC_ACCESSOR(y, 1)
MAKE_VEC_ACCESSOR(z, 2)
#undef MAKE_VEC_ACCESSOR

    vec operator-() const {
        vec res{*this};
        for (auto& e : res.v) e = -e;
        return res;
    }

    vec& operator+=(const vec& rhs) {
        for (unsigned i = 0; i < Size; ++i) v[i] += rhs.v[i];
        return *this;
    }

    vec& operator-=(const vec& rhs) {
        for (unsigned i = 0; i < Size; ++i) v[i] -= rhs.v[i];
        return *this;
    }

    template<typename Y>
    vec& operator*=(Y rhs) {
        for (unsigned i = 0; i < Size; ++i) v[i] *= rhs;
        return *this;
    }
};


namespace detail {
template<typename tag, size_t Size, typename T, std::size_t... I>
constexpr vec<Size, T, tag> make_vec_impl(const std::array<T, Size>& arr, std::index_sequence<I...>) {
    return {arr[I]...};
}
}

template<typename tag, size_t Size, typename T>
constexpr vec<Size, T, tag> make_vec(const std::array<T, Size>& arr) {
    return detail::make_vec_impl<tag>(arr, std::make_index_sequence<Size>());
}

template<unsigned Size, typename T, typename tag>
bool operator==(const vec<Size, T, tag>& l, const vec<Size, T, tag>& r) {
    for (unsigned i = 0; i < Size; ++i) {
        if (l[i] != r[i])
            return false;
    }
    return true;
}

template<unsigned Size, typename T, typename tag>
bool operator!=(const vec<Size, T, tag>& l, const vec<Size, T, tag>& r) {
    return !(l == r);
}

template<unsigned Size, typename T, typename tag, typename Y>
vec<Size, T, tag> operator*(const vec<Size, T, tag>& l, Y r) {
    auto res = l;
    res *= r;
    return res;
}

template<unsigned Size, typename T, typename tag, typename Y>
vec<Size, T, tag> operator*(Y l, const vec<Size, T, tag>& r) {
    auto res = r;
    res *= l;
    return res;
}

template<unsigned Size, typename T, typename tag>
vec<Size, T, tag> operator+(const vec<Size, T, tag>& l, const vec<Size, T, tag>& r) {
    auto res = l;
    res += r;
    return res;
}

template<unsigned Size, typename T, typename tag>
vec<Size, T, tag> operator-(const vec<Size, T, tag>& l, const vec<Size, T, tag>& r) {
    auto res = l;
    res -= r;
    return res;
}

template<unsigned Size, typename T, typename tag>
T dot(const vec<Size, T, tag>& l, const vec<Size, T, tag>& r) {
    T res(0);
    for (unsigned i = 0; i < Size; ++i) {
        res += l[i] * r[i];
    }
    return res;
}

template<unsigned Size, typename T, typename tag>
vec<Size, T, tag> lerp(const vec<Size, T, tag>& a, const vec<Size, T, tag>& b, T t) {
    return a + (b - a) * t;
}

template<unsigned Size, typename T, typename tag>
vec<Size, T, tag> normalized(const vec<Size, T, tag>& v) {
    return v * (1.0f / sqrt(dot(v, v)));
}


} // namespace skirmish

#endif
