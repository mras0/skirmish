#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdint.h>

#include "win32_main_window.h"
#include "d3d11_renderer.h"
#include "vec.h"
#include "mat.h"

#include <fstream>

struct obj_file_contents {
    std::vector<skirmish::world_pos> vertices;
    std::vector<uint16_t>            indices;
};

obj_file_contents load_obj(const std::string& filename)
{
    std::ifstream in{filename};
    if (!in) {
        throw std::runtime_error("Could not open '" + filename + "'");
    }

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


void on_idle()
{
}

int main()
{
    using namespace skirmish;

    try {
#if 1
        const std::string data_dir = "../data/";
        auto obj = load_obj(data_dir + "bunny.obj");

        const auto m = 10.0f * world_mat{
            -1, 0, 0,
            0, 0, 1,
            0, 1, 0};

        //std::transform(begin(obj.vertices), end(obj.vertices), begin(obj.vertices), [&m](const auto& v) { return m * v; });
        for (auto& v: obj.vertices) {
            v = m * v;
        }

#else
        int grid_size = 11;
        std::vector<position> vertices(grid_size * grid_size);
        for (int y = 0; y < grid_size; ++y) {
            for (int x = 0; x < grid_size; ++x) {
                auto& v = vertices[x + y * grid_size];

                const float g2 = grid_size / 2.0f;
                const auto fx = (x - g2) / g2;
                const auto fy = (y - g2) / g2;

                v = position{fx, fy, 0.1f*sin(10*sqrt(fx*fx+fy*fy))};

            }
        }
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
        obj_file_contents obj{vertices, indices};
#endif
        win32_main_window w{640, 480};
        d3d11_renderer renderer{w};
        d3d11_simple_obj render_obj{renderer, make_array_view(obj.vertices), make_array_view(obj.indices)};
        renderer.add_renderable(render_obj);
        w.on_paint([&renderer] {
            world_pos    pos{2, 2, 2};
            world_pos    tgt{0, 0, 0};
            world_normal up{0, 0, 1};
            renderer.set_view(pos, tgt, up);
            renderer.render();
        });
        w.show();

        return run_message_loop(&on_idle);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception caught\n";
        return -1;
    }
}
