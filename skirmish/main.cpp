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
#include <skirmish/math/constants.h>
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

std::vector<simple_vertex> vertices_from_frame(const md3::surface_with_data& surf, unsigned frame)
{
    std::vector<simple_vertex> vs;
    assert(surf.hdr.num_vertices < 65535);
    assert(frame < surf.hdr.num_frames);
    const uint32_t offset = frame*surf.hdr.num_vertices;
    for (uint32_t i = 0; i < surf.hdr.num_vertices; ++i) {
        vs.push_back(simple_vertex{to_world(surf.frames[offset+i].position()), surf.texcoords[i].s, surf.texcoords[i].t});
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

util::path find_file(const std::vector<util::path>& haystack, const std::string& needle)
{
    for (const auto& candidate : haystack) {
        if (simple_tolower(util::path{candidate}.filename()) == needle) {
            return candidate;
        }
    }
    throw std::runtime_error("Could not find " + needle);
}

std::string trim(const std::string& in)
{
    const char* whitespace = "\t\r\n\v ";
    const auto first = in.find_first_not_of(whitespace);
    const auto last  = in.find_last_not_of(whitespace);
    return first != last ? in.substr(first, last-first+1) : "";
}

using skin_info_type = std::map<std::string, std::string>;
skin_info_type read_skin(util::in_stream& in)
{
    assert(trim("") == "");
    assert(trim("\r\n") == "");
    assert(trim("bl ah \tblah") == "bl ah \tblah");
    assert(trim("  \n\r\tbl ah \tblah") == "bl ah \tblah");
    assert(trim("bl ah \tblah  \n\t") == "bl ah \tblah");
    assert(trim("    bl ah \tblah  \n\t") == "bl ah \tblah");

    std::string skin_data(in.stream_size(), '\0');
    in.read(&skin_data[0], skin_data.size());
    if (in.error()) {
        throw std::runtime_error("Error reading skin data");
    }

    std::map<std::string, std::string> res;
    for (size_t pos = 0, size = skin_data.size(); pos < size;) {
        const auto next_eol =  skin_data.find_first_of('\n', pos);
        auto line = skin_data.substr(pos, next_eol-pos);
        const auto comma_pos = line.find_first_of(',');
        if (comma_pos == 0 || comma_pos == std::string::npos) throw std::runtime_error("Invalid skin line: '" + line +"'");
        auto mesh_name = trim(line.substr(0, comma_pos));
        auto texture_filename = trim(line.substr(comma_pos + 1));

        if (!res.insert({std::move(mesh_name), std::move(texture_filename)}).second) {
            throw std::runtime_error("Invalid skin file: '" + mesh_name + "' defined more than once");
        }

        pos = next_eol == std::string::npos ? next_eol : next_eol+1;
    }

    return res;
}

void process_one_md3_file(d3d11_renderer& renderer, std::vector<std::unique_ptr<d3d11_simple_obj>>& objs, util::file_system& fs, const std::string& name, const world_pos& initial_pos) {
    const auto& pk3_files = fs.file_list();
    auto find_in_pk3 = [&pk3_files](const std::string& filename) { return find_file(pk3_files, filename); };        

    const auto skin_filename =  find_in_pk3(name + "_default.skin");
    std::cout << "Loading " << skin_filename << "\n";
    auto skin_info = read_skin(*fs.open(skin_filename));

    const auto md3_filename = find_in_pk3(name + ".md3");
    std::cout << "Loading " << md3_filename << "\n";

    md3::file md3_file;
    if (!read(*fs.open(md3_filename), md3_file)) {
        throw std::runtime_error("Error loading md3 file");
    }

    for (const auto& surf : md3_file.surfaces) {
        std::cout << " " << surf.hdr.name << " " << surf.hdr.num_vertices << " vertices " <<  surf.hdr.num_triangles << " triangles\n";
        objs.push_back(make_obj_from_md3_surface(renderer, surf));
        auto it = skin_info.find(surf.hdr.name);
        if (it != skin_info.end()) {
            const auto texture_filename = it->second;
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
    std::cout << " Frame0 Bounds: " << to_world(md3_file.frames[0].max_bounds) << " " << to_world(md3_file.frames[0].min_bounds) << "\n";
}

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

        std::vector<std::unique_ptr<d3d11_simple_obj>> objs;

        const std::string model_name = "thor";

        zip::in_zip_archive pk3_arc{data_fs.open("md3-"+model_name+".pk3")};
        
        process_one_md3_file(renderer, objs, pk3_arc, "head", {0.0f,0.0f,0.5f});
        process_one_md3_file(renderer, objs, pk3_arc, "upper", {0.0f,0.0f,0.0f});
        process_one_md3_file(renderer, objs, pk3_arc, "lower", {0.0f,0.0f,-0.5f});

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
