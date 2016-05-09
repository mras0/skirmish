#include "file_stream.h"
#include <fstream>
#include <cassert>

namespace skirmish { namespace util {

namespace {

auto path_to_string(const path& p) {
#ifdef _MSC_VER
    return p.c_str();
#else
    // TODO: FIXME
    return p.string();
#endif
}

} // unnamed namespace

class in_file_stream::impl
{
public:
    explicit impl(const path& filename)
        : file_pos_{0}
        , file_size_{invalid_stream_size}
        , in_{path_to_string(filename), std::fstream::binary} {
        if (in_) {
            in_.seekg(0, std::ios_base::end);
            file_size_ = in_.tellg();
            in_.seekg(0, std::ios_base::beg);
        }
    }

    static constexpr int buffer_size = 4096;
    uint8_t       buffer_[buffer_size];
    uint64_t      file_pos_;
    uint64_t      file_size_;
    std::ifstream in_;
};

in_file_stream::in_file_stream(const path& filename) : impl_(new impl(filename))
{
    if (impl_->in_) {
        set_refill(&in_file_stream::refill_in_file_stream);
    } else {
        set_failed(std::make_error_code(std::errc::no_such_file_or_directory));
    }
}

in_file_stream::~in_file_stream() = default;

uint64_t in_file_stream::do_stream_size() const
{
    return impl_->file_size_;
}

void in_file_stream::do_seek(int64_t offset, seekdir way)
{
    assert(!error());
    uint64_t new_file_pos = 0;
    switch (way) {
    case seekdir::beg:
        new_file_pos = offset;
        break;
    case seekdir::cur:
        new_file_pos = tell() + offset;
        break;
    case seekdir::end:
        new_file_pos = impl_->file_size_ + offset;
        break;
    }
    if (new_file_pos <= impl_->file_size_) {
        // TODO: We can optimize when/how the buffer invalidation happens, e.g. if we've seeked inside the current buffer
        // invalidate buffer
        set_buffer(array_view<uint8_t>{});
        impl_->file_pos_ = new_file_pos;
        impl_->in_.seekg(impl_->file_pos_, std::ios_base::beg);
    } else {
        assert(false);
        set_failed(std::make_error_code(std::errc::invalid_seek));
    }
}

uint64_t in_file_stream::do_tell() const
{
    return impl_->file_pos_ + peek().begin() - buffer().begin();
}

array_view<uint8_t> in_file_stream::refill_in_file_stream()
{
    const uint64_t file_remaining = impl_->file_size_ - tell();

    if (!impl_->in_ || !file_remaining) {
        return set_failed(std::make_error_code(std::errc::broken_pipe));
    }

    impl_->file_pos_ += buffer().size();

    impl_->in_.read(reinterpret_cast<char*>(impl_->buffer_), std::min(file_remaining, static_cast<uint64_t>(impl_->buffer_size)));
    const auto byte_count = impl_->in_.gcount();
    if (!byte_count) {
        return set_failed(std::make_error_code(std::errc::io_error));
    }
    return make_array_view(impl_->buffer_, static_cast<size_t>(byte_count));
}

} } // namespace skirmish::util
