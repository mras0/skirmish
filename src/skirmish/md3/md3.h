#ifndef SKIRMISH_MD3_MD3_H
#define SKIRMISH_MD3_MD3_H

#include <skirmish/util/stream.h>
#include <vector>
#include <map>
#include <array>

namespace skirmish { namespace md3 {

static constexpr uint32_t max_name_length = 64;
static constexpr uint32_t max_frames      = 1024;
static constexpr uint32_t max_tags        = 16;
static constexpr uint32_t max_surfaces    = 32;

static constexpr uint32_t magic           = ('3'<<24) | ('P' << 16) | ('D' << 8) | 'I';
static constexpr uint32_t latest_version  = 15;

// Quake (and doom) uses 1 unit = 1 inch = 0.0254 meters
template<typename T>
static constexpr T quake_to_meters_v = T(0.0254);
static constexpr auto quake_to_meters_f = quake_to_meters_v<float>;

struct header {
    uint32_t ident; // IDP3
    uint32_t version;
    char     name[max_name_length];
    uint32_t flags;
    uint32_t num_frames;
    uint32_t num_tags;
    uint32_t num_surfaces;
    uint32_t num_skins;
    uint32_t ofs_frames;
    uint32_t ofs_tags;
    uint32_t ofs_surfaces;
    uint32_t ofs_end;

    bool check() const {
        return ident == magic &&
            version == latest_version &&
            num_frames < max_frames &&
            num_tags < max_tags &&
            num_surfaces < max_surfaces;
    }
};

void read(util::in_stream& in, header& h);

struct vec3 {
    float x, y, z;
};

void read(util::in_stream& in, vec3& v);

struct frame {
    vec3    min_bounds;
    vec3    max_bounds;
    vec3    local_origin;
    float   radius;
    char    name[16];
};

void read(util::in_stream& in, frame& f);

struct tag {
    char name[max_name_length];
    vec3 origin;
    vec3 x_axis;
    vec3 y_axis;
    vec3 z_axis;
};

void read(util::in_stream& in, tag& t);

struct surface {
    uint32_t ident;
    char     name[max_name_length];
    uint32_t flags;
    uint32_t num_frames;
    uint32_t num_shaders;
    uint32_t num_vertices;
    uint32_t num_triangles;
    uint32_t ofs_triangles;
    uint32_t ofs_shaders;
    uint32_t ofs_st;
    uint32_t ofs_xyznormals;
    uint32_t ofs_end;

    bool check() const {
        return ident == magic;
    }
};

void read(util::in_stream& in, surface& s);

struct shader {
    char     name[max_name_length];
    uint32_t index;
};

void read(util::in_stream& in, shader& s);

struct triangle {
    uint32_t a, b, c;
};

void read(util::in_stream& in, triangle& t);

struct texcoord {
    float s, t;
};

void read(util::in_stream& in, texcoord& t);

struct vertex {
    int16_t x, y, z; // x, y, and z coordinates in right-handed 3-space, scaled down by factor 1.0/64. (Multiply by 1.0/64 to obtain original coordinate value.)
    uint8_t nz, na;  // Zenith and azimuth angles of normal vector. 255 corresponds to 2 pi.

    vec3 position() const {
        static constexpr float xyz_scale = 1.0f / 64.0f;
        return vec3{xyz_scale*static_cast<float>(x), xyz_scale*static_cast<float>(y), xyz_scale*static_cast<float>(z)};
    }
};

void read(util::in_stream& in, vertex& v);

struct surface_with_data {
    surface               hdr;
    std::vector<shader>   shaders;
    std::vector<triangle> triangles;
    std::vector<texcoord> texcoords;
    std::vector<vertex>   frames;
};

bool read(util::in_stream& in, surface_with_data& s);

struct file {
    header                          hdr;
    std::vector<frame>              frames;
    std::vector<tag>                tags;
    std::vector<surface_with_data>  surfaces;
};

bool read(util::in_stream& in, file& f);

using skin_info_type = std::map<std::string, std::string>;
skin_info_type read_skin(util::in_stream& in);

enum animation_index {
    BOTH_DEATH1,
    BOTH_DEAD1,
    BOTH_DEATH2,
    BOTH_DEAD2,
    BOTH_DEATH3,
    BOTH_DEAD3,
    TORSO_GESTURE,
    TORSO_ATTACK,
    TORSO_ATTACK2,
    TORSO_DROP,
    TORSO_RAISE,
    TORSO_STAND,
    TORSO_STAND2,
    LEGS_WALKCR,
    LEGS_WALK,
    LEGS_RUN,
    LEGS_BACK,
    LEGS_SWIM,
    LEGS_JUMP,
    LEGS_LAND,
    LEGS_JUMPB,
    LEGS_LANDB,
    LEGS_IDLE,
    LEGS_IDLECR,
    LEGS_TURN,
    MAX_ANIMATION,
};

struct animation_info {
    uint32_t first_frame;
    uint32_t num_frames;
    uint32_t looping_frames;
    uint32_t frames_per_second;
};

using animation_info_array = std::array<animation_info, MAX_ANIMATION>;
animation_info_array read_animation_cfg(util::in_stream& in);

} } // skirmish::md3

#endif
