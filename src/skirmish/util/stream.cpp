#include "stream.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cstring>

namespace skirmish { namespace util {

in_stream::in_stream()
    : start_(nullptr)
    , cursor_(nullptr)
    , end_(nullptr)
    , error_()
    , refill_(&in_stream::zeros)
{
}

array_view<uint8_t> in_stream::peek() const
{
    return make_array_view(cursor_, end_);
}

array_view<uint8_t> in_stream::buffer() const
{
    return make_array_view(start_, end_);
}

void in_stream::set_buffer(const array_view<uint8_t>& av)
{
    start_ = cursor_ = av.begin();
    end_ = av.end();
}

void in_stream::set_cursor(const uint8_t* new_pos)
{
    assert(reinterpret_cast<uintptr_t>(new_pos) >= reinterpret_cast<uintptr_t>(start_));
    assert(reinterpret_cast<uintptr_t>(new_pos) <= reinterpret_cast<uintptr_t>(end_));
    cursor_ = new_pos;
}

void in_stream::ensure_bytes_available()
{
    if (cursor_ >= end_) {
        set_buffer((this->*refill_)());
    }
    assert(peek().size() > 0);
}

void in_stream::read(void* dest, size_t count)
{
    auto dst = static_cast<uint8_t*>(dest);
    while (count) {
        ensure_bytes_available();
        const auto now = std::min(count, static_cast<size_t>(end_ - cursor_));
        std::memcpy(dst, cursor_, now);
        dst     += now;
        cursor_ += now;
        count   -= now;
        assert(cursor_ <= end_);
    }
}

uint8_t in_stream::get()
{
    ensure_bytes_available();
    return *cursor_++;
}

uint16_t in_stream::get_u16_le()
{
    uint16_t res = get();
    res |= static_cast<uint16_t>(get()) << 8;
    return res;
}

uint32_t in_stream::get_u32_le()
{
    uint32_t res = get_u16_le();
    res |= static_cast<uint32_t>(get_u16_le()) << 16;
    return res;
}

float in_stream::get_float_le()
{
    const uint32_t u32 = get_u32_le();
    float f;
    static_assert(sizeof(u32)==sizeof(f),"");
    memcpy(&f, &u32, sizeof(float));
    return f;
}

array_view<uint8_t> in_stream::zeros()
{
    static const uint8_t zeros[256] = {0,};
    return make_array_view(zeros);
}

array_view<uint8_t> in_stream::set_failed(std::error_code error)
{
    assert(error);
    error_  = error;
    refill_ = &in_stream::zeros;
    return zeros();
}

in_zero_stream::in_zero_stream()
{
}

uint64_t in_zero_stream::do_stream_size() const
{
    return invalid_stream_size;
}

void in_zero_stream::do_seek(int64_t, seekdir)
{
}

uint64_t in_zero_stream::do_tell() const
{
    return 0;
}

in_mem_stream::in_mem_stream(const void* data, size_t bytes)
{
    set_buffer(make_array_view(static_cast<const uint8_t*>(data), bytes));
    set_refill(&in_mem_stream::refill_in_mem_stream);
}

array_view<uint8_t> in_mem_stream::refill_in_mem_stream()
{
    return set_failed(std::make_error_code(std::errc::broken_pipe));
}

uint64_t in_mem_stream::do_stream_size() const
{
    assert(!error()); // If the stream has error we're using the zero stream, you probably don't want that
    return buffer().size();
}

void in_mem_stream::do_seek(int64_t offset, seekdir way)
{
    assert(!error()); // If the stream has error we're seeking inside the zero stream, you probably don't want that

    uint64_t new_pos = 0;
    switch (way) {
    case seekdir::beg:
        new_pos = offset;
        break;
    case seekdir::cur:
        new_pos = tell() + offset;
        break;
    case seekdir::end:
        new_pos = stream_size() + offset;
        break;
    }
    if (new_pos <= stream_size()) {
        set_cursor(buffer().begin() + new_pos);
    }
}

uint64_t in_mem_stream::do_tell() const
{
    assert(!error()); // If the stream has error we're using the zero stream, you probably don't want that
    return peek().begin() - buffer().begin();
}

} } // namespace skirmish::util
