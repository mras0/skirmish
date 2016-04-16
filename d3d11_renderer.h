#ifndef SKIRMISH_D3D11_RENDERER_H
#define SKIRMISH_D3D11_RENDERER_H

#include "win32_main_window.h"

namespace skirmish {

class d3d11_create_context;
class d3d11_render_context;

class d3d11_renderable {
public:
    virtual void do_render(d3d11_render_context& context) = 0;
};

class d3d11_renderer;

template<typename T>
class array_view {
public:
    explicit array_view(const T* data, unsigned size) : data_(data), size_(size) {}

    const T* data() const { return data_; }
    unsigned size() const { return size_; }

    const T& operator[](unsigned index) const { return data_[index]; }

private:
    const T* data_;
    unsigned size_;
};

template<typename C>
array_view<typename C::value_type> make_array_view(const C& c) {
    return array_view<typename C::value_type>{ c.data(), static_cast<unsigned>(c.size()) };    
}

class d3d11_simple_obj : public d3d11_renderable {
public:
    explicit d3d11_simple_obj(d3d11_renderer& renderer, const array_view<float>& vertices, const array_view<uint16_t>& indices);
    ~d3d11_simple_obj();
    virtual void do_render(d3d11_render_context& context) override;
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
    void render();
    void add_renderable(d3d11_renderable& r);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace skirmish

#endif