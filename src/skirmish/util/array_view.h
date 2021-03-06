#ifndef SKIRMISH_UTIL_ARRAY_VIEW_H
#define SKIRMISH_UTIL_ARRAY_VIEW_H
#include <cassert>

namespace skirmish { namespace util {

template<typename T>
class array_view {
public:
    explicit constexpr array_view() : data_(nullptr), size_(0) {}

    explicit constexpr array_view(const T* data, size_t size) : data_(data), size_(size) {}

    constexpr const T* data() const { return data_; }
    constexpr size_t   size() const { return size_; }

    constexpr const T& operator[](unsigned index) const { return data_[index]; }

    constexpr const T* begin() const { return data_; }
    constexpr const T* end() const { return data_ + size_; }

private:
    const T* data_;
    size_t   size_;
};

template<typename C>
constexpr array_view<typename C::value_type> make_array_view(const C& c) {
    return array_view<typename C::value_type>{ c.data(), c.size() };    
}

template<typename T>
constexpr array_view<T> make_array_view(const T* data, size_t size) {
    return array_view<T>(data, size);
}

template<typename T>
constexpr array_view<T> make_array_view(const T* begin, const T* end) {
    assert((unsigned long long)begin <= (unsigned long long)end);
    return array_view<T>(begin, end - begin);
}

template<typename T, size_t Size>
constexpr array_view<T> make_array_view(const T (&array)[Size]) {
    return array_view<T>(array, Size);
}

} } // namespace skirmish::util

#endif
