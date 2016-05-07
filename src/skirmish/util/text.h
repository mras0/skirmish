#ifndef SKIRMISH_UTIL_TEXT_H
#define SKIRMISH_UTIL_TEXT_H

#include "stream.h"
#include <string>

namespace skirmish { namespace util {

std::string trim(const std::string& in);

bool read_line(in_stream& s, std::string& line);

} } // namespace skirmish::util

#endif