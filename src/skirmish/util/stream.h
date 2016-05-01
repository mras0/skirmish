#ifndef SKIRMISH_UTIL_STREAM_H
#define SKIRMISH_UTIL_STREAM_H

#include <stdint.h>
#include <system_error>

// Inspired by https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/

namespace skirmish { namespace util {

class in_stream_base {
public:

    void refill();

    std::error_code error() const { return error_; }

    uint8_t get();

protected:
    // start_ <= cursor_ <= end_
    const uint8_t*  start_;
    const uint8_t*  end_;
    const uint8_t*  cursor_;
    std::error_code error_;

    // Must return at least one more byte of data, may set error code
    void (*refill_)(in_stream_base&);

    friend void refill_zero_stream(in_stream_base& s);
    friend void refill_mem_stream(in_stream_base& s);
};

class zero_stream : public in_stream_base {
public:
    explicit zero_stream();
};

class mem_stream : public in_stream_base {
public:
    explicit mem_stream(const void* data, size_t bytes);

    template<typename T, size_t Size>
    explicit mem_stream(const T (&arr)[Size]) : mem_stream(arr, sizeof(T) * Size) {
    }
};

} } // namespace skirmish::util

#endif