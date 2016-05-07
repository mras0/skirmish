#include "md3.h"
#include <skirmish/util/text.h>
#include <cassert>
#include <algorithm>
#include <sstream>

namespace skirmish { namespace md3 {

namespace {

template<typename T>
bool read_to_vec(util::in_stream& in, std::vector<T>& v, uint64_t abs_stream_offset, uint32_t count)
{
    in.seek(abs_stream_offset, util::seekdir::beg);
    v.resize(count);
    for (auto& e : v) {
        read(in, e);
    }
    return !in.error();
}

template<size_t Size>
void read(util::in_stream& in, char (&name)[Size])
{
    static_assert(Size > 0 && Size <= max_name_length, "Invalid array");
    in.read(name, Size);
    assert(std::find(name, name + Size, '\0') != name + Size); // This should hold for all valid MD3 files
    name[Size-1] = '\0'; // Ensure name arrays are always nul-terminated
}

} // unnamed namespace

void read(util::in_stream& in, header& h)
{
    h.ident        = in.get_u32_le();
    h.version      = in.get_u32_le();
    read(in, h.name);
    h.flags        = in.get_u32_le();
    h.num_frames   = in.get_u32_le();
    h.num_tags     = in.get_u32_le();
    h.num_surfaces = in.get_u32_le();
    h.num_skins    = in.get_u32_le();
    h.ofs_frames   = in.get_u32_le();
    h.ofs_tags     = in.get_u32_le();
    h.ofs_surfaces = in.get_u32_le();
    h.ofs_end      = in.get_u32_le();
}

void read(util::in_stream& in, vec3& v)
{
    v.x = in.get_float_le();
    v.y = in.get_float_le();
    v.z = in.get_float_le();
}

void read(util::in_stream& in, frame& f)
{
    read(in, f.min_bounds);
    read(in, f.max_bounds);
    read(in, f.local_origin);
    f.radius = in.get_float_le();
    read(in, f.name);
}

void read(util::in_stream& in, tag& t)
{
    read(in, t.name);
    read(in, t.origin);
    read(in, t.x_axis);
    read(in, t.y_axis);
    read(in, t.z_axis);
}

void read(util::in_stream& in, surface& s)
{
    s.ident          = in.get_u32_le();
    read(in, s.name);
    s.flags          = in.get_u32_le();
    s.num_frames     = in.get_u32_le();
    s.num_shaders    = in.get_u32_le();
    s.num_vertices   = in.get_u32_le();
    s.num_triangles  = in.get_u32_le();
    s.ofs_triangles  = in.get_u32_le();
    s.ofs_shaders    = in.get_u32_le();
    s.ofs_st         = in.get_u32_le();
    s.ofs_xyznormals = in.get_u32_le();
    s.ofs_end        = in.get_u32_le();
}

void read(util::in_stream& in, shader& s)
{
    read(in, s.name);
    s.index = in.get_u32_le();
}

void read(util::in_stream& in, triangle& t)
{
    t.a = in.get_u32_le();
    t.b = in.get_u32_le();
    t.c = in.get_u32_le();
}

void read(util::in_stream& in, texcoord& t)
{
    t.s = in.get_float_le();
    t.t = in.get_float_le();
}

void read(util::in_stream& in, vertex& v)
{
    v.x     = static_cast<int16_t>(in.get_u16_le());
    v.y     = static_cast<int16_t>(in.get_u16_le());
    v.z     = static_cast<int16_t>(in.get_u16_le());
    v.nz    = in.get();
    v.na    = in.get();
}

bool read(util::in_stream& in, surface_with_data& s)
{
    const auto surface_start = in.tell();

    // Header
    read(in, s.hdr);
    if (in.error() || !s.hdr.check()) return false;

    // Read in the order the elements appear in the file to avoid seeking backwards
    enum element { e_shaders, e_triangles, e_texcoords, e_frames };
    std::pair<element, uint32_t> elements[] = {
        { e_shaders   , s.hdr.ofs_shaders    },
        { e_triangles , s.hdr.ofs_triangles  },
        { e_texcoords , s.hdr.ofs_st         },
        { e_frames    , s.hdr.ofs_xyznormals }
    };
    std::sort(std::begin(elements), std::end(elements), [](const auto& l, const auto& r) { return l.second < r.second; });

    for (const auto& e : elements) {
        switch (e.first) {
        case e_shaders:
            if (!read_to_vec(in, s.shaders, surface_start + s.hdr.ofs_shaders, s.hdr.num_shaders)) {
                assert(false);
                return false;
            }
            break;

        case e_triangles:
            if (!read_to_vec(in, s.triangles, surface_start + s.hdr.ofs_triangles, s.hdr.num_triangles)) {
                assert(false);
                return false;
            }
            break;

        case e_texcoords:
            if (!read_to_vec(in, s.texcoords, surface_start + s.hdr.ofs_st, s.hdr.num_vertices)) {
                assert(false);
                return false;
            }
            break;
        case e_frames:
            // Frames (each frame consists of 'num_vertices' vertices)
            if (!read_to_vec(in, s.frames, surface_start + s.hdr.ofs_xyznormals, s.hdr.num_frames * s.hdr.num_vertices)) {
                assert(false);
                return false;
            }
            break;
        }
    }

    // Go to next surface
    in.seek(surface_start + s.hdr.ofs_end, util::seekdir::beg);

    return !in.error();
}

bool read(util::in_stream& in, file& f)
{
    // Header
    read(in, f.hdr);
    if (in.error() || !f.hdr.check()) {
        return false;
    }
    assert(!f.hdr.num_skins);

    // Frames
    if (!read_to_vec(in, f.frames, f.hdr.ofs_frames, f.hdr.num_frames)) {
        assert(false);
        return false;
    }

    // Tags (the tags are animated, so there's one for each frame)
    if (!read_to_vec(in, f.tags, f.hdr.ofs_tags, f.hdr.num_tags * f.hdr.num_frames)) {
        assert(false);
        return false;
    }

    // Surfaces
    if (!read_to_vec(in, f.surfaces, f.hdr.ofs_surfaces, f.hdr.num_surfaces)) {
        assert(false);
        return false;
    }

    // TODO: Verify that the number of frames in each surfaces matches the global value etc.

    return !in.error();
}

skin_info_type read_skin(util::in_stream& in)
{
    skin_info_type res;
    for (std::string line; !in.error() && in.tell() < in.stream_size();) {
        read_line(in, line);
        const auto comma_pos = line.find_first_of(',');
        if (comma_pos == 0 || comma_pos == std::string::npos) throw std::runtime_error("Invalid skin line: '" + line +"'");        
        auto mesh_name = util::trim(line.substr(0, comma_pos));
        auto texture_filename = util::trim(line.substr(comma_pos + 1));

        const auto insert_result = res.insert({std::move(mesh_name), std::move(texture_filename)});
        if (!insert_result.second) {
            throw std::runtime_error("Invalid skin file: '" + insert_result.first->first + "' defined more than once");
        }
    }
    return res;
}

animation_info_array read_animation_cfg(util::in_stream& in)
{
    std::array<animation_info, MAX_ANIMATION> animations{};
    unsigned index = 0;
    for (std::string line; !in.error() && in.tell() < in.stream_size(); ) {
        read_line(in, line);
        line = util::trim(line);
        if (line.empty() || line[0] < '0' || line[0] > '9') continue;
        if (index == MAX_ANIMATION) {
            throw std::runtime_error("Invalid number of animations in animation.cfg");
        }
        animation_info& a = animations[index++];
        std::istringstream iss{line};
        if (!(iss >> a.first_frame >> a.num_frames >> a.looping_frames >> a.frames_per_second)) {
            throw std::runtime_error("Invalid line in animation.cfg: '" + line + "'");
        }
    }
    if (index != MAX_ANIMATION) {
        throw std::runtime_error("Invalid number of animations in animation.cfg: "  + std::to_string(index));
    }

    // From Q3 source: leg only frames are adjusted to not count the upper body only frames
    // Note: calculate adjustment outside the loop as we clobber animations[LEGS_WALKCR] on the first iteration
    const auto legs_frame_adjustment = animations[LEGS_WALKCR].first_frame - animations[TORSO_GESTURE].first_frame;
    for (unsigned index = LEGS_WALKCR; index < MAX_ANIMATION; ++index) {
        animations[index].first_frame -= legs_frame_adjustment;
    }
    return animations;
}


} } // skirmish::md3