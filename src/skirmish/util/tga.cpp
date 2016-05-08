#include "tga.h"
#include <ostream>
#include <cassert>

namespace skirmish { namespace tga {

enum class color_map_type : uint8_t {
    none = 0,
    present = 1
};

void write_grayscale(std::ostream& os, unsigned width, unsigned height, const void* data)
{
    auto put_u8 = [&](uint8_t x) { os.put(static_cast<char>(x)); };
    auto put_u16 = [&](uint16_t x) { put_u8(x&0xff); put_u8(x>>8); };

    put_u8(0);                                                        // ID length
    put_u8(static_cast<uint8_t>(color_map_type::none));               // Color map type
    put_u8(static_cast<uint8_t>(image_type::uncompressed_grayscale)); // Image type
   
    // Color map specification
    put_u16(0); // First entry index
    put_u16(0); // Color map length
    put_u8(0);  // Bits per pixel
    
    // Image specification
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

bool read(util::in_stream& in, image& img)
{
    assert(!in.error());

    const auto id_length  = in.get();
    const auto cmap_type  = static_cast<color_map_type>(in.get());
    const auto img_type   = static_cast<image_type>(in.get());
    // Color map specification
    const auto cmap_first = in.get_u16_le();
    const auto cmap_len   = in.get_u16_le();
    const auto cmap_bpp   = in.get();
    // Image specification
    const auto x_origin   = in.get_u16_le();
    const auto y_origin   = in.get_u16_le();
    const auto width      = in.get_u16_le();
    const auto height     = in.get_u16_le();
    const auto bpp        = in.get();
    const auto alpha_info = in.get();

    if (in.error()) {
        assert(false);
        return false;
    }

    if (cmap_type != color_map_type::none ||
        cmap_len ||
        x_origin ||
        y_origin ||
        width == 0 ||
        height == 0 ||
        img_type != image_type::uncompressed_true_color ||
        (bpp != 24 && bpp !=32)) {
        assert(!"Unsupported image features");
        return false;
    }
    (void) cmap_first; (void) cmap_bpp; (void) alpha_info; // Ignored

    // Skip Image ID
    if (id_length) in.seek(id_length, util::seekdir::cur);
    // No color map
    assert(!cmap_len);
    // Image data
    img.type   = img_type;
    img.width  = width;
    img.height = height;
    img.bpp    = bpp;
    img.data.resize(width * height * ((bpp+7)/8));
    in.read(&img.data[0], img.data.size());
    return !in.error();
}

std::vector<uint32_t> to_rgba(const image& img)
{
    assert(img.type == image_type::uncompressed_true_color && (img.bpp == 24 || img.bpp == 32));
    const int size = img.width * img.height;
    std::vector<uint32_t> res(size);
    switch (img.bpp) {
    case 24:
        for (int i = 0; i < size; ++i) {        
            const uint8_t b = img.data[i *  3 + 0];
            const uint8_t g = img.data[i *  3 + 1];
            const uint8_t r = img.data[i *  3 + 2];
            const uint8_t a = 0xff;
            res[i] = (static_cast<uint32_t>(a)<<24) | (static_cast<uint32_t>(b)<<16) | (static_cast<uint32_t>(g)<<8) | r;
        }
        break;
    case 32:
        for (int i = 0; i < size; ++i) {        
            const uint8_t b = img.data[i *  4 + 0];
            const uint8_t g = img.data[i *  4 + 1];
            const uint8_t r = img.data[i *  4 + 2];
            const uint8_t a = img.data[i *  4 + 3];
            res[i] = (static_cast<uint32_t>(a)<<24) | (static_cast<uint32_t>(b)<<16) | (static_cast<uint32_t>(g)<<8) | r;
        }
        break;
    default:
        assert(false);
    }
    return res;
}

} } // namespace skirmish::tga
