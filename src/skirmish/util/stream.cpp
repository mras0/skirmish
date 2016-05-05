#include "stream.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>

namespace skirmish { namespace util {

in_stream_base::in_stream_base()
    : start_(nullptr)
    , cursor_(nullptr)
    , end_(nullptr)
    , error_()
    , refill_(&in_stream_base::refill_zeros)
{
}

void in_stream_base::refill()
{
    refill_(*this);
}

uint8_t in_stream_base::get()
{
    if (cursor_ >= end_) {
        refill();
    }
    return *cursor_++;
}

uint16_t in_stream_base::get_u16_le()
{
    uint16_t res = get();
    res |= static_cast<uint16_t>(get()) << 8;
    return res;
}

uint32_t in_stream_base::get_u32_le()
{
    uint32_t res = get_u16_le();
    res |= static_cast<uint32_t>(get_u16_le()) << 16;
    return res;
}

void in_stream_base::refill_zeros(in_stream_base& s)
{
    static const uint8_t zeros[256];
    s.start_  = zeros;
    s.cursor_ = s.start_;
    s.end_    = zeros + sizeof(zeros);
}

void in_stream_base::set_failed(std::error_code error)
{
    error_  = error;
    refill_ = &in_stream_base::refill_zeros;
    refill();
}

zero_stream::zero_stream()
{
    refill_ = &in_stream_base::refill_zeros;
    refill();
}

mem_stream::mem_stream(const void* data, size_t bytes)
{
    start_  = reinterpret_cast<const uint8_t*>(data);
    cursor_ = start_;
    end_    = start_ + bytes;
    refill_ = &refill_mem_stream;
}

void mem_stream::refill_mem_stream(in_stream_base& stream)
{
    static_cast<mem_stream&>(stream).set_failed(std::make_error_code(std::errc::broken_pipe));
}

class in_file_stream::impl
{
public:
    explicit impl(const char* filename) : in_{filename, std::fstream::binary} {
    }

    static constexpr int buffer_size = 4096;
    uint8_t       buffer_[buffer_size];
    std::ifstream in_;
};

in_file_stream::in_file_stream(const char* filename) : impl_(new impl(filename))
{
    if (impl_->in_ && impl_->in_.is_open()) {
        refill_ = &in_file_stream::refill_in_file_stream;
        refill();
    } else {
        set_failed(std::make_error_code(std::errc::no_such_file_or_directory));
    }
}

in_file_stream::~in_file_stream() = default;

void in_file_stream::refill_in_file_stream(in_stream_base& stream)
{
    auto& s    = static_cast<in_file_stream&>(stream);
    auto& impl = *s.impl_;

    if (!impl.in_) {
        s.set_failed(std::make_error_code(std::errc::io_error));
        return;
    }

    impl.in_.read(reinterpret_cast<char*>(impl.buffer_), impl.buffer_size);
    const auto byte_count = impl.in_.gcount();
    s.start_  = impl.buffer_;
    s.cursor_ = s.start_;
    s.end_    = s.start_ + byte_count;
}

} } // namespace skirmish::util