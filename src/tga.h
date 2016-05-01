#ifndef SKIRMISH_TGA_H
#define SKIRMISH_TGA_H

#include <iosfwd>
#include <stdint.h>

namespace skirmish { namespace tga {

void write_grayscale(std::ostream& os, unsigned width, unsigned height, const void* data);

} } // namespace skirmish::tga

#endif