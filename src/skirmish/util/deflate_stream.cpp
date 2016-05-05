#include "deflate_stream.h"
#include <cassert>
#include <zlib.h>
#include <string>

namespace skirmish { namespace util {

class zlib_exception : public std::runtime_error {
public:
    zlib_exception(const std::string& desc, int ret) : std::runtime_error(desc + " ret=" + std::to_string(ret)) {
    }
};

class in_deflate_stream::impl {
public:
    explicit impl(in_stream& s) : s_(s), stream_() {
        int ret = inflateInit2(&stream_, -MAX_WBITS); // Initialize inflate in raw mode (no header)
        if (ret != Z_OK) {
            throw zlib_exception("inflateInit failed", ret);
        }
    }

    ~impl() {
        inflateEnd(&stream_);
    }

    array_view<uint8_t> refill() {
        if (!stream_.avail_in) {
            s_.ensure_bytes_available();
            auto in_buf = s_.peek();
            stream_.avail_in = static_cast<uInt>(in_buf.size());
            stream_.next_in  = const_cast<uint8_t*>(in_buf.begin());
        }

        assert(stream_.avail_in);

        stream_.avail_out = buffer_size;
        stream_.next_out  = buffer_;
        int ret = inflate(&stream_, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            throw zlib_exception("zlib inflate failed: " + std::string(stream_.msg), ret);
        }

        return make_array_view(buffer_, buffer_size - stream_.avail_out);
    }

private:
    in_stream&  s_;
    z_stream    stream_;

    static constexpr size_t buffer_size = 16384;
    uint8_t    buffer_[buffer_size];
};

in_deflate_stream::in_deflate_stream(in_stream& inner_stream) : impl_(new impl{inner_stream})
{
    set_refill(&in_deflate_stream::refill_in_deflate_stream);
}

in_deflate_stream::~in_deflate_stream() = default;

array_view<uint8_t> in_deflate_stream::refill_in_deflate_stream()
{
    return impl_->refill();
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