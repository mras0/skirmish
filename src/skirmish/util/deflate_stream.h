#ifndef SKIRMISH_UTIL_DEFLATE_STREAM_H
#define SKIRMISH_UTIL_DEFLATE_STREAM_H

#include "stream.h"

namespace skirmish { namespace util {

class in_deflate_stream : public in_stream {
public:
    explicit in_deflate_stream(in_stream& inner_stream);
    ~in_deflate_stream();

private:
    class impl;
    std::unique_ptr<impl> impl_;

    static void refill_in_deflate_stream(in_stream& s);

    virtual uint64_t do_stream_size() const override;
    virtual void do_seek(int64_t offset, seekdir way) override;
    virtual uint64_t do_tell() const override;
};

} } // namespace skirmish::util

#endif