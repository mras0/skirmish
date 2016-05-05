#ifndef SKIRMISH_UTIL_ZIP_H
#define SKIRMISH_UTIL_ZIP_H

#include <skirmish/util/stream.h>
#include <vector>
#include <string>

namespace skirmish { namespace zip {

class in_zip_archive {
public:
    explicit in_zip_archive(util::in_stream& in);
    ~in_zip_archive();

    std::vector<std::string> filenames() const;

    std::unique_ptr<util::in_stream> get_file_stream(const std::string& filename);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} } // namespace skirmish::zip

#endif