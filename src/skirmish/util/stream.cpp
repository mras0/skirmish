#include "stream.h"

namespace skirmish { namespace util {

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

void refill_zero_stream(in_stream_base& s)
{
    static const uint8_t zeros[256];
    s.start_  = zeros;
    s.cursor_ = s.start_;
    s.end_    = zeros + sizeof(zeros);
}

zero_stream::zero_stream()
{
    error_  = std::error_code{};
    refill_ = &refill_zero_stream;
    refill();
}

void refill_mem_stream(in_stream_base& s)
{
    s.refill_ = &refill_zero_stream;
    s.error_  = std::make_error_code(std::errc::broken_pipe);
    s.refill();
}

mem_stream::mem_stream(const void* data, size_t bytes)
{
    start_  = reinterpret_cast<const uint8_t*>(data);
    cursor_ = start_;
    end_    = start_ + bytes;
    error_  = std::error_code{};
    refill_ = &refill_mem_stream;
}

} } // namespace skirmish::util