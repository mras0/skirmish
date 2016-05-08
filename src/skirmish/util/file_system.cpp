#include "file_system.h"
#include "file_stream.h"

namespace skirmish { namespace util {

namespace {

path strip_root(const path& root, const path& p)
{
    auto root_it  = root.begin();
    auto root_end = root.end();
    auto p_it     = p.begin();
    auto p_end    = p.end();

    // As long as root and p share a common prefix
    while (root_it != root_end && p_it != p_end && *root_it == *p_it) {
        ++root_it;
        ++p_it;
    }

    if (root_it != root_end) {
        throw std::logic_error("strip_root: " + path_to_u8string(p) + " is out side root " + path_to_u8string(root));
    }

    if (p_it == p_end) {
        throw std::logic_error("strip_root: " + path_to_u8string(p) + " is equal to root " + path_to_u8string(root));
    }

    path res;
    for (; p_it != p_end; ++p_it) {
        res /= *p_it;
    }

    return res;
}

} // unnamed namespace

class native_file_system::impl {
public:
    explicit impl(const path& root) : root_(root) {
        if (!exists(root) || !is_directory(root)) {
            throw std::runtime_error(path_to_u8string(root) + " is an invalid path");
        }
    }

    std::vector<path> file_list() const {
        std::vector<path> files;
        for (const auto& p: recursive_directory_iterator{root_}) {
            if (is_regular_file(p)) {
                files.push_back(strip_root(root_, p));
            }
        }
        return files;
    }
    std::unique_ptr<in_stream> open(const path& filename) {
        const auto real_path = root_ / filename;
        assert(exists(real_path) && is_regular_file(real_path));
        return std::make_unique<in_file_stream>(real_path);
    }
private:
    path root_;
};

native_file_system::native_file_system(const path & root) : impl_(new impl{root})
{
}

native_file_system::~native_file_system() = default;

std::vector<path> native_file_system::do_file_list() const
{
    return impl_->file_list();
}

std::unique_ptr<in_stream> native_file_system::do_open(const path & filename)
{
    return impl_->open(filename);
}

} } // namespace skirmish::util
