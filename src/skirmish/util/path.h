#ifndef SKIRMISH_UTIL_PATH_H
#define SKIRMISH_UTIL_PATH_H

#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif

namespace skirmish { namespace util {

#ifdef _MSC_VER
using path = std::experimental::filesystem::path;
using recursive_directory_iterator = std::experimental::filesystem::recursive_directory_iterator;
inline std::string path_to_u8string(const path& p) { return p.generic_u8string(); }
#else
using path = boost::filesystem::path;
using recursive_directory_iterator = boost::filesystem::recursive_directory_iterator;
inline std::string path_to_u8string(const path& p) { return p.generic_string(); } // XXX: FIXME
#endif

} } // namespace skirmish::util

#endif
