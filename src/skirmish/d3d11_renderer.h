#ifndef SKIRMISH_D3D11_RENDERER_H
#define SKIRMISH_D3D11_RENDERER_H

#include "win32_main_window.h"
#include <skirmish/math/types.h>
#include <skirmish/util/array_view.h>

namespace skirmish {

class d3d11_create_context;
class d3d11_render_context;

class d3d11_renderable {
public:
    virtual void do_render(d3d11_render_context& context) = 0;
};

class d3d11_renderer;

class d3d11_simple_obj : public d3d11_renderable {
public:
    explicit d3d11_simple_obj(d3d11_renderer& renderer, const util::array_view<world_pos>& vertices, const util::array_view<uint16_t>& indices);
    ~d3d11_simple_obj();
    virtual void do_render(d3d11_render_context& context) override;

    void update_vertices(const util::array_view<world_pos>& vertices);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

class d3d11_renderer {
public:
    explicit d3d11_renderer(win32_main_window& window);
    d3d11_renderer(const d3d11_renderer&) = delete;
    d3d11_renderer& operator=(const d3d11_renderer&) = delete;
    virtual ~d3d11_renderer();

    d3d11_create_context& create_context();
    void set_view(const world_pos& camera_pos, const world_pos& camera_target);
    void render();
    void add_renderable(d3d11_renderable& r);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace skirmish

#endif
