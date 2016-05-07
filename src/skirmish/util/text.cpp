#include "text.h"
#include <algorithm>

namespace skirmish { namespace util {

std::string trim(const std::string& in)
{
    const char* whitespace = "\t\r\n\v ";
    const auto first = in.find_first_not_of(whitespace);
    const auto last  = in.find_last_not_of(whitespace);
    return first != last ? in.substr(first, last-first+1) : "";
}

bool read_line(in_stream& s, std::string& line)
{
    line.clear();
    for (;;) {
        s.ensure_bytes_available();
        if (s.error()) break;

        // Is there a newline in the buffer?
        const auto& buf = s.peek();
        auto nl = std::find(buf.begin(), buf.end(), '\n');
        if (nl != buf.end()) {
            // Yes, extract all bytes up to it
            line.insert(line.end(), buf.begin(), nl);
            // And move the stream forward to just past it
            s.seek(nl - buf.begin() + 1, seekdir::cur);
            break;
        }
        // No newline in the buffer currently, add all of the data to the line
        line.insert(line.end(), buf.begin(), buf.end());
        // And advance the stream
        s.seek(buf.size(), seekdir::cur);
    }

    return !s.error();
}

} } // namespace skirmish::util