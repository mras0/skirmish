#ifndef SKIRMISH_UTIL_FILE_SYSTEM_H
#define SKIRMISH_UTIL_FILE_SYSTEM_H

#include <vector>
#include "path.h"
#include "stream.h"

namespace skirmish { namespace util {

class file_system {
public:
    virtual ~file_system() {}

    std::vector<path> file_list() {
        return do_file_list();
    }

    std::unique_ptr<in_stream> open(const path& filename) {
        return do_open(filename);
    }

private:
    virtual std::vector<path> do_file_list() const = 0;
    virtual std::unique_ptr<in_stream> do_open(const path& filename) = 0;
};

class native_file_system : public file_system {
public:
    explicit native_file_system(const path& root);
    ~native_file_system();

private:
    class impl;
    std::unique_ptr<impl> impl_;

    virtual std::vector<path> do_file_list() const override;
    virtual std::unique_ptr<in_stream> do_open(const path& filename) override;
};

} } // namespace skirmish::util

#endif