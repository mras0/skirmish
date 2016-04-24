#ifndef SKIRMISH_VEC_H
#define SKIRMISH_VEC_H

#include <array>
#include <utility> // make_index_sequence

namespace skirmish {

template<unsigned Size, typename T, typename tag>
struct vec;

// vec3
template<typename T, typename tag>
struct vec<3, T, tag> {
    T x, y, z;

    constexpr vec operator-() const {
        return { -x, -y, -z };
    }

    vec& operator+=(const vec& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    vec& operator-=(const vec& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    template<typename Y>
    vec& operator*=(Y rhs) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    T& operator[](unsigned index) {
        return index == 0 ? x : index == 1 ? y : z;
    }

    constexpr T operator[](unsigned index) const {
        return index == 0 ? x : index == 1 ? y : index == 2 ? z : throw -1;
    }
};


//
// Generic
//

namespace detail {
template<typename tag, unsigned Size, typename T, std::size_t... I>
constexpr vec<Size, T, tag> make_vec_impl(const std::array<T, Size>& arr, std::index_sequence<I...>) {
    return {arr[I]...};
}
}

template<typename tag, unsigned Size, typename T>
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

} // namespace skirmish

#endif