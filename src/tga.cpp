#include "tga.h"
#include <ostream>

namespace skirmish { namespace tga {

enum class color_map_type : uint8_t {
    none = 0,
    present = 1
};

enum class image_type : uint8_t {
    uncompressed_true_color = 2,
    uncompressed_grayscale = 3 
};

void write_grayscale(std::ostream& os, unsigned width, unsigned height, const void* data)
{
    auto put_u8 = [&](uint8_t x) { os.put(static_cast<char>(x)); };
    auto put_u16 = [&](uint16_t x) { put_u8(x&0xff); put_u8(x>>8); };
    auto put_u32 = [&](uint32_t x) { put_u16(x&0xffff); put_u16(x>>16); };

    put_u8(0);                                                        // ID length
    put_u8(static_cast<uint8_t>(color_map_type::none));               // Color map type
    put_u8(static_cast<uint8_t>(image_type::uncompressed_grayscale)); // Image type
   
    // Color map specification
    put_u16(0); // First entry index
    put_u16(0); // Color map length
    put_u8(0);  // Bits per pixel
    
    // Image specificatio
    put_u16(0);                             // X-origin
    put_u16(0);                             // Y-origin
    put_u16(static_cast<uint16_t>(width));  // Width
    put_u16(static_cast<uint16_t>(height)); // Height
    put_u8(8);                              // Pixel depth (BPP)
    put_u8(0);                              // Image descriptor (1 byte): bits 3-0 give the alpha channel depth, bits 5-4 give direction
    // Image ID
    // Color map
    // Image
    os.write(static_cast<const char*>(data), width * height);
}

} } // namespace skirmish::tga