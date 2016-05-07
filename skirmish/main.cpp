#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <chrono>
#include <stdint.h>
#include <cassert>

#include <skirmish/win32_main_window.h>
#include <skirmish/d3d11_renderer.h>
#include <skirmish/math/constants.h>
#include <skirmish/math/3dmath.h>
#include <skirmish/util/stream.h>
#include <skirmish/util/file_stream.h>
#include <skirmish/util/zip.h>
#include <skirmish/util/path.h>
#include <skirmish/util/perlin.h>
#include <skirmish/util/tga.h>
#include <skirmish/md3/md3.h>
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

std::ostream& operator<<(std::ostream& os, const md3::vec3& v)
{
    return os << "( " << v.x << ", " << v.y << ", " << v.z << ")";
}

world_pos to_world(const md3::vec3& v) {
    return md3::quake_to_meters_f * world_pos{v.x, v.y, v.z};
}

world_normal to_world_normal(const md3::vec3& v) {
    return world_normal{v.x, v.y, v.z};
}

std::vector<simple_vertex> vertices_from_frame(const md3::surface_with_data& surf, unsigned frame)
{
    std::vector<simple_vertex> vs;
    assert(surf.hdr.num_vertices < 65535);
    assert(frame < surf.hdr.num_frames);
    const uint32_t offset = frame*surf.hdr.num_vertices;
    for (uint32_t i = 0; i < surf.hdr.num_vertices; ++i) {
        vs.push_back(simple_vertex{to_world(surf.frames[offset+i].position()), surf.texcoords[i].s, -surf.texcoords[i].t});
    }
    return vs;
}

std::unique_ptr<d3d11_simple_obj> make_obj_from_md3_surface(d3d11_renderer& renderer, const md3::surface_with_data& surf)
{
    std::vector<simple_vertex> vs = vertices_from_frame(surf, 0);
    std::vector<uint16_t>  ts;

    for (uint32_t i = 0; i < surf.hdr.num_triangles; ++i) {
        assert(surf.triangles[i].a < surf.hdr.num_vertices);
        assert(surf.triangles[i].b < surf.hdr.num_vertices);
        assert(surf.triangles[i].c < surf.hdr.num_vertices);
        // Note: reverse order because quake is right-handed
        ts.push_back(static_cast<uint16_t>(surf.triangles[i].c));
        ts.push_back(static_cast<uint16_t>(surf.triangles[i].b));
        ts.push_back(static_cast<uint16_t>(surf.triangles[i].a));
    }

    return std::make_unique<d3d11_simple_obj>(renderer, util::make_array_view(vs), util::make_array_view(ts));
}

using render_obj_vec = std::vector<std::unique_ptr<d3d11_simple_obj>>;

struct animation_instant {
    animation_instant(uint32_t start_frame, uint32_t end_frame, float sub_time) : start_frame(start_frame), end_frame(end_frame), sub_time(sub_time) {
        assert(sub_time >= 0.0f && sub_time <= 1.0f);
    }

    uint32_t start_frame, end_frame;
    float sub_time;
};

animation_instant calc_animation_instant(const md3::animation_info& info, double seconds_since_start)
{
    // XXX: We don't respect the fact that many animations don't actually loop
    const auto animation_pos = fmod(seconds_since_start * info.frames_per_second, info.num_frames);
    const auto frame = static_cast<uint32_t>(animation_pos);
    assert(frame < info.num_frames);
    const auto next_frame = (frame + 1) % info.num_frames;
    return { info.first_frame + frame, info.first_frame + next_frame, static_cast<float>(fmod(animation_pos, 1.0)) };
}

class md3_render_obj {
public:
    explicit md3_render_obj(d3d11_renderer& renderer, util::file_system& fs, const std::string& base_name) {
        auto skin_filename = base_name;
        skin_filename += "_default.skin";

        auto md3_filename = base_name;
        md3_filename += ".md3";
        std::cout << "Loading " << md3_filename << "\n";

        if (!read(*fs.open(md3_filename), file_)) {
            throw std::runtime_error("Error loading md3 file");
        }

        std::cout << "Loading " << skin_filename << "\n";
        skin_info_ = md3::read_skin(*fs.open(skin_filename));

        for (const auto& surf : file_.surfaces) {
            std::cout << " Surface " << surf.hdr.name << " " << surf.hdr.num_vertices << " vertices " <<  surf.hdr.num_triangles << " triangles\n";
            surfaces_.push_back(make_obj_from_md3_surface(renderer, surf));

            auto it = skin_info_.find(surf.hdr.name);
            if (it != skin_info_.end()) {
                const auto& texture_filename = it->second;
                std::cout << " Loading texture: " << texture_filename << std::endl;
                tga::image img;
                if (!tga::read(*fs.open(texture_filename), img)) {
                    throw std::runtime_error("Could not load TGA " + texture_filename);
                }
                std::cout << "  " << img.width << " x " << img.height << std::endl;
                d3d11_texture tex(renderer, util::make_array_view(tga::to_rgba(img)), img.width, img.height);
                surfaces_.back()->set_texture(tex);
            }
            renderer.add_renderable(*surfaces_.back());
        }
    }

    const md3::file& file() const {
        return file_;
    }

    const md3::tag& tag(const std::string& name, uint32_t frame) const {
        assert(frame < file_.hdr.num_frames);
        for (unsigned i = 0; i < file_.hdr.num_tags; ++i) {
            const auto& t = file_.tags[frame * file_.hdr.num_tags + i];
            if (t.name == name) {
                return t;
            }
        }
        throw std::runtime_error("Tag " + name + " not found for frame " + std::to_string(frame));
    }

    void set_transform(const world_transform& transform) {
        for (auto& s : surfaces_) {
            s->set_world_transform(transform);
        }
    }

    void update_animation(const animation_instant& ai) {
        assert(ai.start_frame < file_.hdr.num_frames);
        assert(ai.end_frame < file_.hdr.num_frames);
        assert(file_.surfaces.size() == surfaces_.size());
        for (size_t surfnum = 0; surfnum < surfaces_.size(); ++surfnum) {
            auto f0 = vertices_from_frame(file_.surfaces[surfnum], ai.start_frame);
            auto f1 = vertices_from_frame(file_.surfaces[surfnum], ai.end_frame);
            assert(f0.size() == f1.size());
            for (size_t vertnum = 0; vertnum < f0.size(); ++vertnum) {
                f0[vertnum].pos = lerp(f0[vertnum].pos, f1[vertnum].pos, ai.sub_time);
                // S and T don't change between frames
            }
            surfaces_[surfnum]->update_vertices(util::make_array_view(f0));
        }
    }

private:
    md3::file           file_;
    md3::skin_info_type skin_info_;
    render_obj_vec      surfaces_;
};


world_transform lerp(const md3::tag& a, const md3::tag& b, float t)
{
    // Linear interpolation of the individual axes and the renormalizing was good enough for quake
    // but we might want to do slerp on quaternions later on
    const auto origin = lerp(to_world(a.origin), to_world(b.origin), t);
    const auto x      = normalized(lerp(to_world_normal(a.x_axis), to_world_normal(b.x_axis), t));
    const auto y      = normalized(lerp(to_world_normal(a.y_axis), to_world_normal(b.y_axis), t));
    const auto z      = normalized(lerp(to_world_normal(a.z_axis), to_world_normal(b.z_axis), t));
    return world_transform {
        x.x(), y.x(), z.x(), origin.x(),
        x.y(), y.y(), z.y(), origin.y(),
        x.z(), y.z(), z.z(), origin.z(),
            0,     0,     0,          1
    };
}

world_transform animate_tag(const md3_render_obj& obj, const animation_instant& ai, const char* tag)
{
    return lerp(obj.tag(tag, ai.start_frame), obj.tag(tag, ai.end_frame), ai.sub_time);
}

class q3_player_render_obj {
public:
    explicit q3_player_render_obj(d3d11_renderer& renderer, util::file_system& fs, const std::string& base_path)
        : head_ (renderer, fs, base_path + "/head")
        , torso_(renderer, fs, base_path + "/upper")
        , legs_ (renderer, fs, base_path + "/lower")
        , animation_info_(md3::read_animation_cfg(*fs.open(base_path + "/animation.cfg"))) {
    }

    void update(double t) {
        auto rot = world_transform::factory::rotation_z(static_cast<float>(t * 3));
        auto legs_transform  = rot*world_transform::factory::translation({0.0f,0.0f, 0.0f}); 
        update_transforms(legs_transform, t);
    }

private:
    static constexpr const char* const head_tag  = "tag_head";
    static constexpr const char* const torso_tag = "tag_torso";

    void update_transforms(const world_transform& legs_transform, double t) {
        const auto torso_ai = calc_animation_instant(animation_info_[md3::TORSO_ATTACK], t);
        const auto legs_ai  = calc_animation_instant(animation_info_[md3::LEGS_WALK], t);

        torso_.update_animation(torso_ai);
        legs_.update_animation(legs_ai);

        auto torso_transform = legs_transform * animate_tag(legs_, legs_ai, torso_tag);
        auto head_transform  = torso_transform * animate_tag(torso_, torso_ai, head_tag);
        head_.set_transform(head_transform);
        torso_.set_transform(torso_transform);
        legs_.set_transform(legs_transform);
    }

    md3_render_obj              head_;
    md3_render_obj              torso_;
    md3_render_obj              legs_;
    md3::animation_info_array   animation_info_;
};

/*
void process_one_md3_file(d3d11_renderer& renderer, std::vector<std::unique_ptr<d3d11_simple_obj>>& objs, util::file_system& fs, const util::path& name, const world_pos& initial_pos) {
    md3_object md3_obj;
    read_md3_object(fs, md3_obj, name);

    for (const auto& surf : md3_obj.f.surfaces) {
        std::cout << " " << surf.hdr.name << " " << surf.hdr.num_vertices << " vertices " <<  surf.hdr.num_triangles << " triangles\n";
        objs.push_back(make_obj_from_md3_surface(renderer, surf));
        const auto texture_filename = md3_obj.surface_texture(surf.hdr.name);        
        if (!texture_filename.empty()) {
            std::cout << " Loading texture: " << texture_filename << std::endl;
            tga::image img;
            if (!tga::read(*fs.open(texture_filename), img)) {
                throw std::runtime_error("Could not load TGA " + texture_filename);
            }
            std::cout << "  " << img.width << " x " << img.height << std::endl;
            d3d11_texture tex(renderer, util::make_array_view(tga::to_rgba(img)), img.width, img.height);
            objs.back()->set_texture(tex);
        }
        auto world_mat = world_transform::identity();
        world_mat[0][3] = initial_pos.x();
        world_mat[1][3] = initial_pos.y();
        world_mat[2][3] = initial_pos.z();
        objs.back()->set_world_transform(world_mat);
        renderer.add_renderable(*objs.back());
    }
}
*/

int main()
{
    try {
        const std::string data_dir = "../../data/";
        util::native_file_system data_fs{data_dir};
        
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

        const std::string model_name = "mario";
        zip::in_zip_archive pk3_arc{data_fs.open("md3-"+model_name+".pk3")};
        q3_player_render_obj q3player{renderer, pk3_arc, "models/players/"+model_name};

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
        world_pos camera_pos{2.0f, 0.0f, 1.0f};
        float view_ang = -pi_f/2.0f;//-pi_f;
        w.on_paint([&] {
            using clock = std::chrono::high_resolution_clock;
            static auto start = clock::now();
            const auto t = std::chrono::duration_cast<std::chrono::duration<double>>(clock::now() - start).count();
            static auto last  = t;
            q3player.update(t);

            view_ang += static_cast<float>((t - last) * 2.5 * (key_down[key::left] * -1 + key_down[key::right] * 1));
            world_pos view_vec{sinf(view_ang), -cosf(view_ang), 0.0f};

            camera_pos += view_vec * static_cast<float>((t - last) * 5 * (key_down[key::up] * 1 + key_down[key::down] * -1));

            auto camera_target = camera_pos + view_vec;
            camera_target[2] = 0.5f;

            std::ostringstream oss;
            oss << camera_pos << " " << camera_target << " viewdir: " << view_ang;
            w.set_title(oss.str());

            renderer.set_view(camera_pos, camera_target);
            renderer.render();
            last = t;
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
