#ifndef SKIRMISH_OBJ_OBJ_H
#define SKIRMISH_OBJ_OBJ_H

#include <vector>

namespace skirmish { namespace util {
class in_stream;
} }

namespace skirmish { namespace obj {

struct vertex {
    double x, y, z;
};

struct file {
    std::vector<vertex>   vertices;
    std::vector<uint16_t> indices;
};

bool read(util::in_stream& in, file& f);

} } // namespace skirmish::obj

#endif
