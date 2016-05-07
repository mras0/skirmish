#ifndef SKIRMISH_WIN32_Q3_PLAYER_RENDER_OBJ_H
#define SKIRMISH_WIN32_Q3_PLAYER_RENDER_OBJ_H

#include <skirmish/util/file_system.h>
#include <skirmish/win32/d3d11_renderer.h>

namespace skirmish {

class q3_player_render_obj {
public:
    q3_player_render_obj(d3d11_renderer& renderer, util::file_system& fs, const std::string& base_path);
    ~q3_player_render_obj();

    void update(double t);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace skirmish

#endif