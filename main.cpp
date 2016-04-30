#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <cassert>

#include "win32_main_window.h"
#include "d3d11_renderer.h"
#include "vec.h"
#include "mat.h"
#include "perlin.h"
#include "tga.h"

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

int main()
{
    using namespace skirmish;

    try {
        const std::string data_dir = "../data/";
        auto bunny = load_obj(data_dir + "bunny.obj");

        const auto m = 10.0f * world_mat{
            -1, 0, 0,
            0, 0, 1,
            0, 1, 0};

        std::transform(begin(bunny.vertices), end(bunny.vertices), begin(bunny.vertices), [&m](const auto& v) { return m * v; });
        
        constexpr int grid_size = 150;
        constexpr float grid_scale = 5;
        std::vector<world_pos> vertices(grid_size * grid_size);
        std::vector<uint8_t> height_map(grid_size*grid_size);
        for (int y = 0; y < grid_size; ++y) {
            for (int x = 0; x < grid_size; ++x) {
                const auto index  = x + y * grid_size;
                const auto fx = static_cast<float>(x) / grid_size;
                const auto fy = static_cast<float>(y) / grid_size;
                const auto fz = perlin_noise_2d(fx, fy, 0.65f, 8);
                assert(fz >= 0.0f && fz <= 1.0f);
                height_map[index] = static_cast<uint8_t>(255.0f * fz);
                vertices[index] = world_pos{fx * grid_scale, fy * grid_scale, fz};
            }
        }

        tga::write_grayscale(open_file::binary_out("height.tga"), grid_size, grid_size, height_map.data());

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
        }
        obj_file_contents terrain{vertices, indices};

        win32_main_window w{640, 480};
        std::map<key, bool> key_down;

        w.on_key_down([&](key k) {
            key_down[k] = true; 
            if (k == key::escape) w.close();
        });
        w.on_key_up([&](key k) { key_down[k] = false; });

        d3d11_renderer renderer{w};
        d3d11_simple_obj bunny_obj{renderer, make_array_view(bunny.vertices), make_array_view(bunny.indices)};
        d3d11_simple_obj terrain_obj{renderer, make_array_view(terrain.vertices), make_array_view(terrain.indices)};
        renderer.add_renderable(bunny_obj);
        renderer.add_renderable(terrain_obj);

        world_pos camera_pos{grid_scale/2.0f, grid_scale/2.0f, 4};
        float view_ang = -pi_f;
        w.on_paint([&] {
            view_ang += 0.001f * (key_down[key::left] * -1 + key_down[key::right] * 1);
            world_pos view_vec{sinf(view_ang), -cosf(view_ang), 0.0f};

            camera_pos += view_vec * (0.01f * (key_down[key::up] * 1 + key_down[key::down] * -1));

            auto camera_target = camera_pos + view_vec;
            camera_target[2] = 0;

            std::ostringstream oss;
            oss << camera_pos << " " << camera_target;
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
