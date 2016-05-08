#include "deflate_stream.h"
#include <cassert>
#include <zlib.h>
#include <string>
#include <algorithm>

namespace skirmish { namespace util {

class zlib_exception : public std::runtime_error {
public:
    zlib_exception(const std::string& desc, int ret) : std::runtime_error(desc + " ret=" + std::to_string(ret)) {
    }
};

class in_deflate_stream::impl {
public:
    explicit impl(in_stream& s, size_t compressed_size, size_t uncompressed_size)
        : s_(s)
        , compressed_size_(compressed_size)
        , uncompressed_size_(uncompressed_size)
        , crc32_(::crc32(0, nullptr, 0))
        , stream_() {
        assert(!s.error());
        int ret = inflateInit2(&stream_, -MAX_WBITS); // Initialize inflate in raw mode (no header)
        if (ret != Z_OK) {
            throw zlib_exception("inflateInit failed", ret);
        }
    }

    ~impl() {
        inflateEnd(&stream_);
    }

    array_view<uint8_t> refill() {
        assert(!stream_.avail_out);
        const auto avail_out_on_start = static_cast<uInt>(std::min(buffer_size, uncompressed_size_ - stream_.total_out));
        stream_.avail_out = avail_out_on_start;
        stream_.next_out  = buffer_;

        do {
            assert(!end_reached());

            if (!stream_.avail_in) {
                assert(stream_.total_in < compressed_size_);
                s_.ensure_bytes_available();
                auto in_buf = s_.peek();
                stream_.avail_in = static_cast<uInt>(std::min(in_buf.size(), compressed_size_ - stream_.total_in));
                stream_.next_in  = const_cast<uint8_t*>(in_buf.begin());
                s_.seek(stream_.avail_in, seekdir::cur); // mark bytes as consumed
            }

            assert(stream_.avail_in);
            assert(stream_.avail_out);

            int ret = inflate(&stream_, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                throw zlib_exception("zlib inflate failed: " + std::string(stream_.msg ? stream_.msg : "unknown error"), ret);
            }

            if (ret == Z_STREAM_END) {
                if (!end_reached()) {
                    assert(false);
                    // TODO: Turn this into a soft error?
                    throw std::runtime_error("Premature EOF in deflate stream");
                }
            }
        } while (!end_reached() && stream_.avail_out);

        auto buf = make_array_view(buffer_, avail_out_on_start - stream_.avail_out);
        crc32_ = ::crc32(crc32_, buf.begin(), static_cast<uInt>(buf.size()));

        return buf;
    }

    uint32_t crc32() const {
        assert(end_reached());
        return crc32_;
    }

    bool end_reached() const {
        return stream_.total_out >= uncompressed_size_;
    }

private:
    in_stream&  s_;
    size_t      compressed_size_;
    size_t      uncompressed_size_;
    uint32_t    crc32_;
    z_stream    stream_;

    static constexpr size_t buffer_size = 16384;
    uint8_t    buffer_[buffer_size];
};
constexpr size_t in_deflate_stream::impl::buffer_size;

in_deflate_stream::in_deflate_stream(in_stream& inner_stream, size_t compressed_size, size_t uncompressed_size) : impl_(new impl{inner_stream, compressed_size, uncompressed_size})
{
    set_refill(&in_deflate_stream::refill_in_deflate_stream);
}

in_deflate_stream::~in_deflate_stream() = default;

uint32_t in_deflate_stream::crc32() const
{
    return impl_->crc32();
}

array_view<uint8_t> in_deflate_stream::refill_in_deflate_stream()
{
    if (impl_->end_reached()) {
        return set_failed(std::make_error_code(std::errc::broken_pipe));
    }
    return impl_->refill();
}

uint64_t in_deflate_stream::do_stream_size() const
{
    assert(false);
    return invalid_stream_size;
}

void in_deflate_stream::do_seek(int64_t offset, seekdir way)
{
    if (way == seekdir::cur && offset >= 0 && static_cast<uint64_t>(offset) <= peek().size()) {
        set_cursor(peek().begin() + offset);
    } else {
        assert(false);
        set_failed(std::make_error_code(std::errc::invalid_seek));
    }
}

uint64_t in_deflate_stream::do_tell() const
{
    assert(false);
    return invalid_stream_size;
}

} } // namespace skirmish::util
