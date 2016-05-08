#include "q3_player_render_obj.h"

#include <skirmish/math/types.h>
#include <skirmish/math/3dmath.h>
#include <skirmish/util/file_system.h>
#include <skirmish/util/tga.h>
#include <skirmish/md3/md3.h>

#include <skirmish/win32/d3d11_renderer.h>

namespace skirmish {

namespace { 
world_pos to_world(const md3::vec3& v) {
    return md3::quake_to_meters_f * world_pos{v.x, v.y, v.z};
}

world_pos to_world_normal(const md3::vec3& v) {
    return world_pos{v.x, v.y, v.z};
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
        //std::cout << "Loading " << md3_filename << "\n";

        if (!read(*fs.open(md3_filename), file_)) {
            throw std::runtime_error("Error loading md3 file");
        }

        //std::cout << "Loading " << skin_filename << "\n";
        skin_info_ = md3::read_skin(*fs.open(skin_filename));

        for (const auto& surf : file_.surfaces) {
            //std::cout << " Surface " << surf.hdr.name << " " << surf.hdr.num_vertices << " vertices " <<  surf.hdr.num_triangles << " triangles\n";
            surfaces_.push_back(make_obj_from_md3_surface(renderer, surf));

            auto it = skin_info_.find(surf.hdr.name);
            if (it != skin_info_.end()) {
                const auto& texture_filename = it->second;
                //std::cout << " Loading texture: " << texture_filename << std::endl;
                tga::image img;
                if (!tga::read(*fs.open(texture_filename), img)) {
                    throw std::runtime_error("Could not load TGA " + texture_filename);
                }
                //std::cout << "  " << img.width << " x " << img.height << std::endl;
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

} // unnamed namespace

class q3_player_render_obj::impl {
public:
    explicit impl(d3d11_renderer& renderer, util::file_system& fs, const std::string& base_path)
        : head_ (renderer, fs, base_path + "/head")
        , torso_(renderer, fs, base_path + "/upper")
        , legs_ (renderer, fs, base_path + "/lower")
        , animation_info_(md3::read_animation_cfg(*fs.open(base_path + "/animation.cfg"))) {
    }

    void update(double t, const world_transform& legs_transform) {
        const auto torso_ai = calc_animation_instant(animation_info_[md3::TORSO_ATTACK], t);
        const auto legs_ai  = calc_animation_instant(animation_info_[md3::LEGS_WALKCR], t);

        torso_.update_animation(torso_ai);
        legs_.update_animation(legs_ai);

        auto torso_transform = legs_transform * animate_tag(legs_, legs_ai, torso_tag);
        auto head_transform  = torso_transform * animate_tag(torso_, torso_ai, head_tag);
        head_.set_transform(head_transform);
        torso_.set_transform(torso_transform);
        legs_.set_transform(legs_transform);
    }

private:
    static constexpr const char* const head_tag  = "tag_head";
    static constexpr const char* const torso_tag = "tag_torso";

    md3_render_obj              head_;
    md3_render_obj              torso_;
    md3_render_obj              legs_;
    md3::animation_info_array   animation_info_;
};

q3_player_render_obj::q3_player_render_obj(d3d11_renderer& renderer, util::file_system& fs, const std::string& base_path)
    : impl_(new impl{renderer, fs, base_path})
{
}

q3_player_render_obj::~q3_player_render_obj() = default;

void q3_player_render_obj::update(double t, const world_transform& transform)
{
    impl_->update(t, transform);
}

} // namespace skirmish