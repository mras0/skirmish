#ifndef SKIRMISH_UTIL_ZIP_H
#define SKIRMISH_UTIL_ZIP_H

#include <skirmish/util/stream.h>
#include <skirmish/util/file_system.h>
#include <vector>
#include <string>

namespace skirmish { namespace zip {

class in_zip_archive : public util::file_system {
public:
    explicit in_zip_archive(util::in_stream& in);
    explicit in_zip_archive(std::unique_ptr<util::in_stream> in);
    ~in_zip_archive();

private:
    class impl;
    std::unique_ptr<impl> impl_;

    virtual std::vector<util::path> do_file_list() const override;
    virtual std::unique_ptr<util::in_stream> do_open(const util::path& filename) override;
};

} } // namespace skirmish::zip

#endif