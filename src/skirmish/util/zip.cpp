#include "zip.h"
#include "zip_internals.h"
#include "deflate_stream.h"
#include <map>
#include <algorithm>
#include <cassert>

namespace skirmish { namespace zip {

class in_zip_file_stream : public util::in_stream {
public:
    explicit in_zip_file_stream(util::in_stream& zip, bool& open, bool raw, size_t compressed_size, size_t uncompressed_size, uint32_t crc32)
        : zip_(zip)
        , deflate_(raw ? nullptr : new util::in_deflate_stream{zip, compressed_size, uncompressed_size})
        , open_(open)
        , pos_(0)
        , size_(uncompressed_size)
        , crc32_(crc32) {
        set_refill(&in_zip_file_stream::refill_in_zip_file_stream);
    }

    ~in_zip_file_stream() {
        assert(open_);
        open_ = false;
    }

private:
    util::in_stream&                         zip_;
    std::unique_ptr<util::in_deflate_stream> deflate_;
    bool&                                    open_;
    size_t                                   pos_;
    size_t                                   size_;
    uint32_t                                 crc32_;

    util::array_view<uint8_t> refill_in_zip_file_stream() {
        if (pos_ >= size_) {
            return set_failed(std::make_error_code(std::errc::broken_pipe));
        }

        auto& uncompressed_ = deflate_ ? *deflate_ : zip_;

        assert(!uncompressed_.error());
        pos_ += buffer().size();

        uncompressed_.seek(buffer().size(), util::seekdir::cur);
        uncompressed_.ensure_bytes_available();
        if (uncompressed_.error()) {
            return set_failed(std::make_error_code(std::errc::io_error));
        }
        auto buf = uncompressed_.peek();
        if (pos_ + buf.size() >= size_) {
            // TODO: Check CRC32 on uncompressed file
            if (deflate_ && deflate_->crc32() != crc32_) {
                assert(false);
                return set_failed(std::make_error_code(std::errc::io_error));
            }
        }
        return buf;
    }

    virtual uint64_t do_stream_size() const override {
        return size_;
    }

    virtual void do_seek(int64_t offset, util::seekdir way) override {
        if (way == util::seekdir::beg) {
            offset -= tell();
        } else if (way == util::seekdir::end) {
            offset = stream_size() - offset - tell();
        }

        if (offset < 0) {
            assert(false);
            set_failed(std::make_error_code(std::errc::invalid_seek));
            return;
        }

        // Discard 'offset' bytes
#if 1
        while (offset) {
            ensure_bytes_available();
            const auto now = std::min(peek().size(), static_cast<uint64_t>(offset));
            set_cursor(peek().begin() + now);
            offset -= now;
        }
#else
        while (offset--) get();
#endif
    }

    virtual uint64_t do_tell() const override {
        return pos_ + peek().begin() - buffer().begin();
    }
};


class in_zip_archive::impl {
public:
    explicit impl(util::in_stream& in) : zip_(in), open_file_(false) {
        if (zip_.error()) {
            throw std::system_error(zip_.error(), "Invalid ZIP stream");
        }
        const auto central_dir_end_pos = find_end_of_central_directory_record(zip_, dir_end_);
        if (central_dir_end_pos == invalid_file_pos) {
            throw std::runtime_error("Invalid zip archive");
        }

        if (dir_end_.disk_number != 0 || dir_end_.central_disk != 0) {
            throw std::runtime_error("Disk-spanning zip archives not supported");
        }

        const auto central_dir_end = dir_end_.central_directory_offset + dir_end_.central_directory_size_bytes;

        if (central_dir_end > zip_.stream_size() || central_dir_end > central_dir_end_pos) {
            throw std::runtime_error("Central directory in zip file is invalid");
        }

        zip_.seek(dir_end_.central_directory_offset, util::seekdir::beg);

        while (!zip_.error() && zip_.tell() < central_dir_end) {
            central_directory_file_header central_header;
            read(zip_, central_header);

            if (central_header.signature != central_directory_file_header::signature_magic) {
                throw std::runtime_error("Invalid central header in zip file");
            }

            std::string filename(central_header.filename_length, '\0');
            zip_.read(&filename[0], filename.size());
            zip_.seek(central_header.extra_field_length, util::seekdir::cur);
            zip_.seek(central_header.file_comment_length, util::seekdir::cur);
            const bool is_dir = (central_header.external_file_attributes & 0x10) != 0;
            if (!is_dir) {
                const auto ir = files_.insert({std::move(filename), central_header});
                if (!ir.second) {
                    throw std::runtime_error("Duplicate filename in zip: " + ir.first->first);
                }
            }
        }

        if (zip_.error()) {
            throw std::runtime_error("Error while reading zip central directory");
        }
    }

    std::vector<std::string> filenames() const {
        std::vector<std::string> res;
        for (const auto& f: files_) {
            res.push_back(f.first);
        }
        return res;
    }

    std::unique_ptr<util::in_stream> get_file_stream(const std::string& filename) {
        assert(!zip_.error());
        if (open_file_) {
            assert(false);
            throw std::runtime_error("Only one file can be open at a time");
        }

        auto it = files_.find(filename);
        if (it == files_.end()) {
            throw std::runtime_error(filename + " not found in zip archive");
        }

        const auto& ch = it->second;
        zip_.seek(ch.local_file_header_offset, util::seekdir::beg);
        local_file_header lh;
        read(zip_, lh);
        if (zip_.error() || lh.signature != local_file_header::signature_magic) {
            throw std::runtime_error("Could not read local file header for " + filename);
        }

        if (ch.min_version != lh.min_version ||
            ch.flags != lh.flags ||
            ch.compression_method != lh.compression_method ||
            ch.last_modified_time != lh.last_modified_time ||
            ch.last_modified_date != lh.last_modified_date ||
            ch.crc32 != lh.crc32 ||
            ch.compressed_size != lh.compressed_size ||
            ch.uncompressed_size != lh.uncompressed_size ||
            ch.filename_length != lh.filename_length) {
            throw std::runtime_error("Local and central file headers differ for " + filename);
        }
        // TODO: Could check filenames as well here...
        zip_.seek(lh.filename_length + lh.extra_field_length, util::seekdir::cur);
        assert(ch.compression_method == compression_methods::stored || ch.compression_method == compression_methods::deflated);
        open_file_ = true;
        return std::make_unique<in_zip_file_stream>(zip_, open_file_, ch.compression_method == compression_methods::stored, ch.compressed_size, ch.uncompressed_size, ch.crc32);
    }

private:
    util::in_stream&                                        zip_;
    end_of_central_directory_record                         dir_end_;
    std::map<std::string, central_directory_file_header>    files_;
    bool                                                    open_file_;
};

in_zip_archive::in_zip_archive(util::in_stream& in) : impl_(new impl{in})
{
}

in_zip_archive::~in_zip_archive() = default;

std::vector<std::string> in_zip_archive::filenames() const
{
    return impl_->filenames();
}

std::unique_ptr<util::in_stream> in_zip_archive::get_file_stream(const std::string& filename)
{
    return impl_->get_file_stream(filename);
}

} } // namespace skirmish::zip