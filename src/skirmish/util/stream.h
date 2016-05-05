#ifndef SKIRMISH_UTIL_STREAM_H
#define SKIRMISH_UTIL_STREAM_H

#include <stdint.h>
#include <system_error>
#include <memory>

#include <skirmish/util/array_view.h>

// Inspired by https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/

namespace skirmish { namespace util {

enum class seekdir {
    beg, cur, end
};

static constexpr uint64_t invalid_stream_size = UINT64_MAX;

class in_stream {
public:
    virtual ~in_stream() {}

    // Returns the streams current error condition (or a default constructed error_code if no error)
    const std::error_code& error() const { return error_; }

    // Returns peek at buffer, might be empty
    array_view<uint8_t> peek() const;

    // Makes sure atleast one byte is available for consumption (post condition: peek().size() > 0)
    void ensure_bytes_available();

    // Read count bytes
    void read(void* dest, size_t count);

    // Read one byte
    uint8_t get();

    // Read little-endian 16-bit value
    uint16_t get_u16_le();

    // Read little-endian 32-bit value
    uint32_t get_u32_le();

    // Returns stream size (may or may not be meaningful for the stream)
    uint64_t stream_size() const {
        return do_stream_size();
    }

    // Seeks in the stream (behavior depends on the stream type)
    void seek(int64_t offset, seekdir way) {
        do_seek(offset, way);
    }

    // Returns current stream position (may or may not be meaningful for the stream)
    uint64_t tell() const {
        return do_tell();
    }

protected:
    explicit in_stream();

    // start_ <= cursor_ <= end_
    const uint8_t*  start_;
    const uint8_t*  end_;
    const uint8_t*  cursor_;

    // Must return at least one more byte of data, may set error code
    void (*refill_)(in_stream&);

    static void refill_zeros(in_stream& s);

    void refill();
    void set_failed(std::error_code error);

    virtual uint64_t do_stream_size() const = 0;
    virtual void do_seek(int64_t offset, seekdir way) = 0;
    virtual uint64_t do_tell() const = 0;

private:
    // starts out default constructed, errors are sticky
    std::error_code error_;
};

class in_zero_stream : public in_stream {
public:
    explicit in_zero_stream();

private:
    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

class in_mem_stream : public in_stream {
public:
    explicit in_mem_stream(const void* data, size_t bytes);

    template<typename T, size_t Size>
    explicit in_mem_stream(const T (&arr)[Size]) : in_mem_stream(arr, sizeof(T) * Size) {
        static_assert(sizeof(T) == 1, "Untested");
    }

    template<typename T>
    explicit in_mem_stream(const array_view<T>& av) : in_mem_stream(av.data(), sizeof(T) * av.size()) {
        static_assert(sizeof(T) == 1, "Untested");
    }

private:
    static void refill_in_mem_stream(in_stream& s);

    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

class in_file_stream : public in_stream {
public:
    explicit in_file_stream(const char* filename);
    ~in_file_stream();

private:
    class impl;
    std::unique_ptr<impl> impl_;

    static void refill_in_file_stream(in_stream& s);

    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

} } // namespace skirmish::util

#endif