#ifndef SKIRMISH_TGA_H
#define SKIRMISH_TGA_H

#include <iosfwd>
#include <stdint.h>
#include <skirmish/util/stream.h>
#include <vector>

namespace skirmish { namespace tga {

void write_grayscale(std::ostream& os, unsigned width, unsigned height, const void* data);

enum class image_type : uint8_t {
    uncompressed_true_color = 2,
    uncompressed_grayscale = 3 
};

struct image {
    image_type           type;
    uint16_t             width;
    uint16_t             height;
    uint8_t              bpp;
    std::vector<uint8_t> data;
};

bool read(util::in_stream& in, image& img);

std::vector<uint32_t> to_rgba(const image& img);

} } // namespace skirmish::tga

#endif