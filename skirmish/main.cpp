#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <cassert>

#include <skirmish/win32_main_window.h>
#include <skirmish/d3d11_renderer.h>
#include <skirmish/perlin.h>
#include <skirmish/tga.h>
#include <skirmish/math/constants.h>
#include <skirmish/util/stream.h>
#include <skirmish/util/zip.h>
#include <skirmish/util/path.h>

#include <fstream>

class open_file {
public:
    static open_file text_in(const std::string& filename) {
        return open_file{filename, std::ios_base::in};
    }

    static open_file binary_out(const std::string& filename) {
        return open_file{filename, std::ios_base::binary | std::ios_base::out};
    }

    operator std::istream&() {
        return s_;
    }

    operator std::ostream&() {
        return s_;
    }

private:
    explicit open_file(const std::string& filename, std::ios_base::openmode mode) : s_{filename.c_str(), mode} {
        if (!s_) {
            throw std::runtime_error("Could not open '" + filename + "'");
        }
    }

    std::fstream s_;
};

template<unsigned Size, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const skirmish::vec<Size, T, tag>& v) {
    os << "(";
    for (unsigned i = 0; i < Size; ++i) {
        os << " " << v[i];
    }
    return os << " )";
}

struct obj_file_contents {
    std::vector<skirmish::world_pos> vertices;
    std::vector<uint16_t>            indices;
};

obj_file_contents load_obj(const std::string& filename)
{
    auto f = open_file::text_in(filename);
    std::istream& in = f;

    obj_file_contents res;

    for (std::string line; std::getline(in, line);) {
        std::istringstream iss{line};

        auto err = [&] (const std::string& msg) {
            throw std::runtime_error("Invalid obj file '" + filename + "'. Line: '" + line + "'. Error: " + msg);
        };

        char element;
        if (!(iss >> element)) {
            err("Empty line");
        }
        if (element == '#') { // comment
            continue;
        } else if (element == 'v') { // geometric vertex 
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                err("Incomplete vertex line");
            }
            float w;
            if ((iss >> w) && w != 1.0f) {
                err("w != 1 not supported");
            }
            res.vertices.push_back({x, y, z});
        } else if (element == 'f') { // Polygonal face element
            unsigned long long a, b, c;
            if (!(iss >> a >> b >> c)) {
                err("Incomplete polygonal face line");
            }
            unsigned long long d;
            if (iss >> d) {
                err("Only triangles supported");
            }

            if (a == 0 || b == 0 || c == 0) {
                err("Invalid indices");
            }
            a--;
            b--;
            c--;
            
            if (a > UINT16_MAX || b > UINT16_MAX || c > UINT16_MAX) {
                err("Only 16-bit indices supported");
            }

            res.indices.push_back(static_cast<uint16_t>(a));
            res.indices.push_back(static_cast<uint16_t>(b));
            res.indices.push_back(static_cast<uint16_t>(c));
        } else {
            err("Unknown element found");
        }
    }

    return res;
}

using namespace skirmish;

auto calc_grid(int grid_size, float prescale, float grid_scale, float persistence, int number_of_octaves)
{
    std::vector<world_pos> vertices(grid_size * grid_size);
    std::vector<uint8_t> height_map(grid_size * grid_size);
    for (int y = 0; y < grid_size; ++y) {
        for (int x = 0; x < grid_size; ++x) {
            const auto index  = x + y * grid_size;
            const auto fx = static_cast<float>(x) / grid_size;
            const auto fy = static_cast<float>(y) / grid_size;
            const auto fz = perlin::noise_2d(prescale*fx, prescale*fy, persistence, number_of_octaves);

#ifndef NDEBUG
            if (fz < 0 || fz > 1) {
                std::cerr << "\n" << fz << "\n";
                abort();
            }
#endif
            height_map[index] = static_cast<uint8_t>(255.0f * fz);
            vertices[index] = world_pos{fx*grid_scale, fy*grid_scale, fz};
        }
    }
    tga::write_grayscale(open_file::binary_out("height.tga"), grid_size, grid_size, height_map.data());
    return vertices;
};

namespace skirmish { namespace md3 {

static constexpr uint32_t max_name_length = 64;
static constexpr uint32_t magic           = ('3'<<24) | ('P' << 16) | ('D' << 8) | 'I';

// Quake (and doom) uses 1 unit = 1 inch = 0.0254 meters
template<typename T>
static constexpr T quake_to_meters_v = T(0.0254);
static constexpr auto quake_to_meters_f = quake_to_meters_v<float>;

struct header {
    static constexpr uint32_t md3_version = 15;

    static constexpr uint32_t max_frames   = 1024;
    static constexpr uint32_t max_tags     = 16;
    static constexpr uint32_t max_surfaces = 32;

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
        return ident == magic && version == md3_version && num_frames < max_frames && num_tags < max_tags && num_surfaces < max_surfaces;
    }
};

void read(util::in_stream& in, header& h)
{
    h.ident        = in.get_u32_le();
    h.version      = in.get_u32_le();
    in.read(h.name, sizeof(h.name));
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

struct vec3 {
    float x, y, z;
};

void read(util::in_stream& in, vec3& v)
{
    v.x = in.get_float_le();
    v.y = in.get_float_le();
    v.z = in.get_float_le();
}

std::ostream& operator<<(std::ostream& os, const vec3& v)
{
    return os << "( " << v.x << ", " << v.y << ", " << v.z << ")";
}

struct frame {
    vec3    min_bounds;
    vec3    max_bounds;
    vec3    local_origin;
    float   radius;
    char    name[16];
};

void read(util::in_stream& in, frame& f)
{
    read(in, f.min_bounds);
    read(in, f.max_bounds);
    read(in, f.local_origin);
    f.radius = in.get_float_le();
    in.read(f.name, sizeof(f.name));
}

struct tag {
    char name[max_name_length];
    vec3 origin;
    vec3 x_axis;
    vec3 y_axis;
    vec3 z_axis;
};

void read(util::in_stream& in, tag& t)
{
    in.read(t.name, sizeof(t.name));
    read(in, t.origin);
    read(in, t.x_axis);
    read(in, t.y_axis);
    read(in, t.z_axis);
}

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

void read(util::in_stream& in, surface& s)
{
    s.ident          = in.get_u32_le();
    in.read(s.name, sizeof(s.name));
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

struct shader {
    char     name[max_name_length];
    uint32_t index;
};

void read(util::in_stream& in, shader& s)
{
    in.read(s.name, sizeof(s.name));
    s.index = in.get_u32_le();
}

struct triangle {
    uint32_t a, b, c;
};

void read(util::in_stream& in, triangle& t)
{
    t.a = in.get_u32_le();
    t.b = in.get_u32_le();
    t.c = in.get_u32_le();
}

struct texcoord {
    float s, t;
};

void read(util::in_stream& in, texcoord& t)
{
    t.s = in.get_float_le();
    t.t = in.get_float_le();
}

struct vertex {
    int16_t x, y, z; // x, y, and z coordinates in right-handed 3-space, scaled down by factor 1.0/64. (Multiply by 1.0/64 to obtain original coordinate value.)
    uint8_t nz, na;  // Zenith and azimuth angles of normal vector. 255 corresponds to 2 pi.

    vec3 position() const {
        static constexpr float xyz_scale = 1.0f / 64.0f;
        return vec3{xyz_scale*static_cast<float>(x), xyz_scale*static_cast<float>(y), xyz_scale*static_cast<float>(z)};
    }
};

void read(util::in_stream& in, vertex& v)
{
    v.x     = static_cast<int16_t>(in.get_u16_le());
    v.y     = static_cast<int16_t>(in.get_u16_le());
    v.z     = static_cast<int16_t>(in.get_u16_le());
    v.nz    = in.get();
    v.na    = in.get();
}

struct surface_with_data {
    surface               hdr;
    std::vector<shader>   shaders;
    std::vector<triangle> triangles;
    std::vector<texcoord> texcoords;
    std::vector<vertex>   frames;
};

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

struct file {
    header                          hdr;
    std::vector<frame>              frames;
    std::vector<tag>                tags;
    std::vector<surface_with_data>  surfaces;
};

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

    // Tags
    if (!read_to_vec(in, f.tags, f.hdr.ofs_tags, f.hdr.num_tags)) {
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

} } // skirmish::md3

world_pos to_world(const md3::vec3& v) {
    return md3::quake_to_meters_f * world_pos{v.x, v.y, v.z};
}

std::unique_ptr<d3d11_simple_obj> make_obj_from_md3_surface(d3d11_renderer& renderer, const md3::surface_with_data& surf)
{
    std::vector<simple_vertex> vs;
    std::vector<uint16_t>  ts;

    assert(surf.hdr.num_vertices < 65535);
    for (uint32_t i = 0; i < surf.hdr.num_vertices; ++i) {
        vs.push_back(simple_vertex{to_world(surf.frames[i].position()), surf.texcoords[i].s, surf.texcoords[i].t});
    }

    for (uint32_t i = 0; i < surf.hdr.num_triangles; ++i) {
        assert(surf.triangles[i].a < surf.hdr.num_vertices);
        assert(surf.triangles[i].b < surf.hdr.num_vertices);
        assert(surf.triangles[i].c < surf.hdr.num_vertices);
        ts.push_back(static_cast<uint16_t>(surf.triangles[i].c));
        ts.push_back(static_cast<uint16_t>(surf.triangles[i].b));
        ts.push_back(static_cast<uint16_t>(surf.triangles[i].a));
    }

    return std::make_unique<d3d11_simple_obj>(renderer, util::make_array_view(vs), util::make_array_view(ts));
}

std::string simple_tolower(const util::path& p)
{
    std::string s = p.string();
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
    }
    return s;
}

std::string find_file(const std::vector<std::string>& haystack, const std::string& needle)
{
    for (const auto& candidate : haystack) {
        if (simple_tolower(util::path{candidate}.filename()) == needle) {
            return candidate;
        }
    }
    throw std::runtime_error("Could not find " + needle);
}

std::map<std::string, std::string> read_skin(util::in_stream& in)
{
    std::string skin_data(in.stream_size(), '\0');
    in.read(&skin_data[0], skin_data.size());
    if (in.error()) {
        throw std::runtime_error("Error reading skin data");
    }

    std::map<std::string, std::string> res;
    for (size_t pos = 0, size = skin_data.size(); pos < size;) {
        const auto next_eol =  skin_data.find_first_of('\n', pos);
        auto line = skin_data.substr(pos, next_eol-pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        //std::cout << "Line: " << line << '\n';
        const auto comma_pos = line.find_first_of(',');
        if (comma_pos == 0 || comma_pos == std::string::npos) throw std::runtime_error("Invalid skin line: '" + line +"'");
        auto mesh_name = line.substr(0, comma_pos);
        auto texture_filename = line.substr(comma_pos + 1);

        if (!res.insert({std::move(mesh_name), std::move(texture_filename)}).second) {
            throw std::runtime_error("Invalid skin file: '" + mesh_name + "' defined more than once");
        }

        pos = next_eol == std::string::npos ? next_eol : next_eol+1;
    }

    return res;
}

int main()
{
    try {
        const std::string data_dir = "../../data/";
        
        /*
        auto bunny = load_obj(data_dir + "bunny.obj");

        const auto m = 10.0f * world_mat{
            -1, 0, 0,
            0, 0, 1,
            0, 1, 0};

        std::transform(begin(bunny.vertices), end(bunny.vertices), begin(bunny.vertices), [&m](const auto& v) { return m * v; });

        constexpr int grid_size = 250;
        static_assert(grid_size * grid_size <= 65335, "Grid too large");

        constexpr float grid_scale = grid_size / 5.0f;

        float persistence = 0.45f;
        int number_of_octaves = 9;
        float prescale = grid_scale;

        std::vector<world_pos> vertices;

        auto recalc_grid = [&] {
            std::cout << "calc_grid(grid_size=" << grid_size << ", prescale=" << prescale << ", grid_scale=" << grid_scale << ", persistence=" << persistence << ", number_of_octaves=" << number_of_octaves << "\n";
            vertices = calc_grid(grid_size, prescale, grid_scale, persistence, number_of_octaves);
        };
        recalc_grid();

        std::vector<uint16_t> indices;
        for (int y = 0; y < grid_size-1; ++y) {
            for (int x = 0; x < grid_size-1; ++x) {
                int idx = x + y * grid_size;
                indices.push_back(static_cast<uint16_t>(idx));
                indices.push_back(static_cast<uint16_t>(idx+1));
                indices.push_back(static_cast<uint16_t>(idx+1+grid_size));

                indices.push_back(static_cast<uint16_t>(idx+1+grid_size));
                indices.push_back(static_cast<uint16_t>(idx+grid_size));
                indices.push_back(static_cast<uint16_t>(idx));
            }
        }*/

        win32_main_window w{640, 480};
        std::map<key, bool> key_down;        

        d3d11_renderer renderer{w};
        //d3d11_simple_obj bunny_obj{renderer, util::make_array_view(bunny.vertices), util::make_array_view(bunny.indices)};
        //d3d11_simple_obj terrain_obj{renderer, util::make_array_view(vertices), util::make_array_view(indices)};
        //renderer.add_renderable(bunny_obj);
        //renderer.add_renderable(terrain_obj);

        std::vector<std::unique_ptr<d3d11_simple_obj>> objs;
        util::in_file_stream pk3_stream{data_dir + "md3-ange.pk3"};
        zip::in_zip_archive pk3_arc{pk3_stream};

        auto process_one_md3_file = [&] (const std::string& name) {
            const auto& pk3_files = pk3_arc.filenames();
            auto find_in_pk3 = [&pk3_files](const std::string& filename) { return find_file(pk3_files, filename); };        

            const auto skin_filename =  find_in_pk3(name + "_default.skin");
            std::cout << "Loading " << skin_filename << "\n";
            auto skin_info = read_skin(*pk3_arc.get_file_stream(skin_filename));

            const auto md3_filename = find_in_pk3(name + ".md3");
            std::cout << "Loading " << md3_filename << "\n";
            auto md3_stream = pk3_arc.get_file_stream(md3_filename);

            md3::file md3_file;
            if (!read(*md3_stream, md3_file)) {
                throw std::runtime_error("Error loading md3 file");
            }
            md3_stream.reset();

            for (const auto& surf : md3_file.surfaces) {
                std::cout << " " << surf.hdr.name << " " << surf.hdr.num_vertices << " vertices " <<  surf.hdr.num_triangles << " triangles\n";
                objs.push_back(make_obj_from_md3_surface(renderer, surf));
                auto it = skin_info.find(surf.hdr.name);
                if (it != skin_info.end()) {
                    const auto texture_filename = simple_tolower(util::path{it->second}.filename());
                    std::cout << " Loading texture: " << texture_filename << std::endl;
                    auto tga_stream = pk3_arc.get_file_stream(find_in_pk3(texture_filename));
                    tga::image img;
                    if (!tga::read(*tga_stream, img)) {
                        throw std::runtime_error("Could not load TGA " + texture_filename);
                    }
                    std::cout << "  " << img.width << " x " << img.height << std::endl;
                    d3d11_texture tex(renderer, util::make_array_view(tga::to_rgba(img)), img.width, img.height);
                    objs.back()->set_texture(tex);
                }
                renderer.add_renderable(*objs.back());
            }
            std::cout << " Frame0 Bounds: " << to_world(md3_file.frames[0].max_bounds) << " " << to_world(md3_file.frames[0].min_bounds) << "\n";
        };
        
        process_one_md3_file("head");
        process_one_md3_file("lower");
        process_one_md3_file("upper");

        w.on_key_down([&](key k) {
            key_down[k] = true; 
            /*
            bool needs_to_recalc_grid = false;
            constexpr float persistence_step = 0.05f;
            constexpr float prescale_step = 1;
            if (k == key::a && number_of_octaves < perlin::max_number_of_octaves) {
                ++number_of_octaves;
                needs_to_recalc_grid = true;
            } else if (k == key::z && number_of_octaves > 1) {
                --number_of_octaves;
                needs_to_recalc_grid = true;
            } else if (k == key::s && persistence + persistence_step <= 1.0f) {
                persistence += persistence_step;
                needs_to_recalc_grid = true;
            } else if (k == key::x && persistence - persistence_step >= 0.0f) {
                persistence -= persistence_step;
                needs_to_recalc_grid = true;
            } else if (k == key::d) {
                prescale += prescale_step;
                needs_to_recalc_grid = true;
            } else if (k == key::c && prescale - prescale_step >= 1) {
                prescale -= prescale_step;
                needs_to_recalc_grid = true;
            }

            if (needs_to_recalc_grid) {
                recalc_grid();
                terrain_obj.update_vertices(util::make_array_view(vertices));
            }*/
        });
        w.on_key_up([&](key k) {
            key_down[k] = false;

            if (k == key::escape) {
                w.close();
            }
        });

        //world_pos camera_pos{grid_scale/2.0f, grid_scale/2.0f, 2.0f};
        world_pos camera_pos{1.0f, 1.0f, 1.0f};
        float view_ang = -0.5f;//-pi_f;
        w.on_paint([&] {
            view_ang += 0.0025f * (key_down[key::left] * -1 + key_down[key::right] * 1);
            world_pos view_vec{sinf(view_ang), -cosf(view_ang), 0.0f};

            camera_pos += view_vec * (0.01f * (key_down[key::up] * 1 + key_down[key::down] * -1));

            auto camera_target = camera_pos + view_vec;
            camera_target[2] = 0;

            std::ostringstream oss;
            oss << camera_pos << " " << camera_target << " viewdir: " << view_ang;
            w.set_title(oss.str());

            renderer.set_view(camera_pos, camera_target);
            renderer.render();
        });
        w.show();

        return run_message_loop([]{/*on_idle*/});
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception caught\n";
        return -1;
    }
}
