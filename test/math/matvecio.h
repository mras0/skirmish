#ifndef MATVECIO_H
#define MATVECIO_H

#include <skirmish/math/vec.h>
#include <skirmish/math/mat.h>
#include <ostream>

template<unsigned Size, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::vec<Size, T, tag>& v) {
    os << "(";
    for (unsigned i = 0; i < Size; ++i) {
        os << " " << v[i];
    }
    return os << " )";
}

template<unsigned Rows, unsigned Columns, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::mat<Rows, Columns, T, tag>& m) {
    for (unsigned r = 0; r < Rows; ++r) {
        os << m[r] << '\n';
    }
    return os;
}

#endif