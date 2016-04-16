#ifndef SKIRMISH_WIN32_H
#define SKIRMISH_WIN32_H

#include <memory>
#include <functional>

namespace skirmish {

class d3d11_window {
public:
    explicit d3d11_window();
    ~d3d11_window();
    d3d11_window(const d3d11_window&) = delete;
    d3d11_window& operator=(const d3d11_window&) = delete;

    void show();

private:
    class impl;
    std::unique_ptr<impl> impl_;
};


using on_idle_func = std::function<void (void)>;
int run_message_loop(const on_idle_func& on_idle);

}

#endif