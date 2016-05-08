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

#include <skirmish/math/constants.h>
#include <skirmish/math/3dmath.h>
#include <skirmish/util/stream.h>
#include <skirmish/util/file_stream.h>
#include <skirmish/util/zip.h>
#include <skirmish/util/path.h>
#include <skirmish/util/perlin.h>
#include <skirmish/util/tga.h>
#include <skirmish/obj/obj.h>
#include <skirmish/md3/md3.h>
#include <skirmish/win32/win32_main_window.h>
#include <skirmish/win32/d3d11_renderer.h>
#include <skirmish/win32/q3_player_render_obj.h>

using namespace skirmish;

template<unsigned Size, typename T, typename tag>
std::ostream& operator<<(std::ostream& os, const vec<Size, T, tag>& v) {
    os << "(";
    for (unsigned i = 0; i < Size; ++i) {
        os << " " << v[i];
    }
    return os << " )";
}

std::unique_ptr<d3d11_simple_obj> make_terrian_obj(d3d11_renderer& renderer)
{
    constexpr int grid_size = 250;
    static_assert(grid_size * grid_size <= 65335, "Grid too large");

    constexpr float grid_scale = grid_size / 5.0f;

    float persistence = 0.45f;
    int number_of_octaves = 9;
    float prescale = grid_scale;

    std::vector<simple_vertex> vertices(grid_size * grid_size);
    for (int y = 0; y < grid_size; ++y) {
        for (int x = 0; x < grid_size; ++x) {
            const auto index  = x + y * grid_size;
            const auto fx = static_cast<float>(x) / grid_size;
            const auto fy = static_cast<float>(y) / grid_size;
            const auto fz = perlin::noise_2d(prescale*fx, prescale*fy, persistence, number_of_octaves);
            assert(fz >= 0.0f && fz <= 1.0f);
            vertices[index].pos = world_pos{fx*grid_scale, fy*grid_scale, fz};
            vertices[index].s = fx;
            vertices[index].t = fy;
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
    return std::make_unique<d3d11_simple_obj>(renderer, util::make_array_view(vertices), util::make_array_view(indices));
}

std::unique_ptr<d3d11_simple_obj> load_obj_for_render(d3d11_renderer& renderer, util::in_stream& in)
{
    obj::file obj;
    read(in, obj);
    std::vector<simple_vertex> vertices;
    for (const auto& bv : obj.vertices) {
        const auto pos = world_pos{static_cast<float>(-bv.x), static_cast<float>(bv.z), static_cast<float>(bv.y)};
        const float scale = 5.0f;
        vertices.push_back({scale*pos, 0.0f, 0.0f});
    }
    return std::make_unique<d3d11_simple_obj>(renderer, util::make_array_view(vertices), util::make_array_view(obj.indices));
}

int main()
{
    try {
        util::native_file_system data_fs{"../../data/"};
        
        win32_main_window w{640, 480};

        d3d11_renderer renderer{w};

        auto bunny = load_obj_for_render(renderer, *data_fs.open("bunny.obj"));
        bunny->set_world_transform(world_transform::factory::translation({1,1,0}));
        renderer.add_renderable(*bunny);

        auto terrain_obj = make_terrian_obj(renderer);
        static const uint32_t terrain_tex[] = { 0xffffffff, 0xff0000ff, 0xff00ff00, 0xffff0000} ;
        d3d11_texture tex(renderer, util::make_array_view(terrain_tex), 2, 2);
        terrain_obj->set_texture(tex);
        renderer.add_renderable(*terrain_obj);

        const std::string model_name = "mario";
        zip::in_zip_archive pk3_arc{data_fs.open("md3-"+model_name+".pk3")};
        q3_player_render_obj q3player{renderer, pk3_arc, "models/players/"+model_name};

        std::map<key, bool> key_down;
        w.on_key_down([&](key k) {
            key_down[k] = true;        
        });
        w.on_key_up([&](key k) {
            key_down[k] = false;

            if (k == key::escape) {
                w.close();
            }
        });

        world_pos camera_pos{2.0f, 0.0f, 2.0f};
        float view_ang = -pi_f/2.0f;
        w.on_paint([&] {
            // Time handling
            using clock = std::chrono::high_resolution_clock;
            static auto start = clock::now();
            const auto t = std::chrono::duration_cast<std::chrono::duration<double>>(clock::now() - start).count();
            static auto last_t = t;
            const auto dt = t - last_t;
            last_t = t;

            // Update according to movement
            view_ang += static_cast<float>(dt * 2.5 * (key_down[key::left] * -1 + key_down[key::right] * 1));
            world_pos view_vec{sinf(view_ang), -cosf(view_ang), 0.0f};
            camera_pos += view_vec * static_cast<float>(dt * 5 * (key_down[key::up] * 1 + key_down[key::down] * -1));
            auto camera_target = camera_pos + view_vec;
            camera_target[2] = 0.7f;

            // Update render stuff
            q3player.update(t, world_transform::factory::translation(camera_target) * world_transform::factory::rotation_z(view_ang - pi_f/2.0f));

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
