#ifndef SKIRMISH_UTIL_FILE_STREAM_H
#define SKIRMISH_UTIL_FILE_STREAM_H

#include "stream.h"
#include "path.h"

namespace skirmish { namespace util {

class in_file_stream : public in_stream {
public:
    explicit in_file_stream(const path& filename);
    ~in_file_stream();

private:
    class impl;
    std::unique_ptr<impl> impl_;

    array_view<uint8_t> refill_in_file_stream();

    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

} } // namespace skirmish::util

#endif