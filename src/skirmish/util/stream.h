#ifndef SKIRMISH_UTIL_STREAM_H
#define SKIRMISH_UTIL_STREAM_H

#include <stdint.h>
#include <system_error>
#include <memory>

// Inspired by https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/

namespace skirmish { namespace util {

enum class seekdir {
    beg, cur, end
};

static constexpr uint64_t invalid_stream_size = UINT64_MAX;

class in_stream_base {
public:
    virtual ~in_stream_base() {}

    std::error_code error() const { return error_; }

    uint8_t get();

    uint16_t get_u16_le();
    uint32_t get_u32_le();

    uint64_t stream_size() const {
        return do_stream_size();
    }

    void seek(int64_t offset, seekdir way) {
        do_seek(offset, way);
    }

    uint64_t tell() const {
        return do_tell();
    }

protected:
    explicit in_stream_base();

    // start_ <= cursor_ <= end_
    const uint8_t*  start_;
    const uint8_t*  end_;
    const uint8_t*  cursor_;
    std::error_code error_;

    // Must return at least one more byte of data, may set error code
    void (*refill_)(in_stream_base&);

    static void refill_zeros(in_stream_base& s);

    void refill();
    void set_failed(std::error_code error);

    virtual uint64_t do_stream_size() const = 0;
    virtual void do_seek(int64_t offset, seekdir way) = 0;
    virtual uint64_t do_tell() const = 0;
};

class zero_stream : public in_stream_base {
public:
    explicit zero_stream();

private:
    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

class mem_stream : public in_stream_base {
public:
    explicit mem_stream(const void* data, size_t bytes);

    template<typename T, size_t Size>
    explicit mem_stream(const T (&arr)[Size]) : mem_stream(arr, sizeof(T) * Size) {
    }

private:
    static void refill_mem_stream(in_stream_base& s);

    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

class in_file_stream : public in_stream_base {
public:
    explicit in_file_stream(const char* filename);
    ~in_file_stream();

private:
    class impl;
    std::unique_ptr<impl> impl_;

    static void refill_in_file_stream(in_stream_base& s);

    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

} } // namespace skirmish::util

#endif