#ifndef SKIRMISH_VEC_H
#define SKIRMISH_VEC_H

namespace skirmish {

template<typename T, typename tag>
struct vec3 {
    T x, y, z;

    vec3& operator*=(T rhs) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }
};

template<typename T, typename tag>
bool operator==(const vec3<T, tag>& l, const vec3<T, tag>& r) {
    return l.x == r.x && l.y == r.y && l.z == r.z;
}

template<typename T, typename tag>
bool operator!=(const vec3<T, tag>& l, const vec3<T, tag>& r) {
    return !(l == r);
}

template<typename tag>
using vec3f = vec3<float, tag>;


} // namespace skirmish

#endif