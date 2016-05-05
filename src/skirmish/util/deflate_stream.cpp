#include "deflate_stream.h"
#include <cassert>

namespace skirmish { namespace util {

class in_deflate_stream::impl {
public:
    explicit impl(in_stream& inner_stream) {
        (void)inner_stream;
    }

private:
};

in_deflate_stream::in_deflate_stream(in_stream& inner_stream) : impl_(new impl{inner_stream})
{
    refill_ = &in_deflate_stream::refill_in_deflate_stream;
}

in_deflate_stream::~in_deflate_stream() = default;

void in_deflate_stream::refill_in_deflate_stream(in_stream& s)
{
    assert(false);
    (void)s;
}

uint64_t in_deflate_stream::do_stream_size() const
{
    assert(false);
    return invalid_stream_size;
}

void in_deflate_stream::do_seek(int64_t offset, seekdir way)
{
    assert(false);
    (void)offset; (void)way;
}

uint64_t in_deflate_stream::do_tell() const
{
    assert(false);
    return invalid_stream_size;
}

} } // namespace skirmish::util