#ifndef SKIRMISH_WIN32_win32_main_window_H
#define SKIRMISH_WIN32_win32_main_window_H

#include <memory>
#include <functional>

struct HWND__;

namespace skirmish {

enum class key {
    escape,
    left, right, up, down,

// Must be last
    one_past_max
};

class win32_main_window {
public:
    using native_handle_type = HWND__*;

    explicit win32_main_window(unsigned width, unsigned height);
    ~win32_main_window();
    win32_main_window(const win32_main_window&) = delete;
    win32_main_window& operator=(const win32_main_window&) = delete;

    native_handle_type native_handle();

    void show();
    void close();

    void set_title(const std::string& text);

    using on_paint_func = std::function<void (void)>;
    on_paint_func on_paint() const;
    void on_paint(const on_paint_func& on_paint);

    using on_key_down_func = std::function<void (key)>;
    on_key_down_func on_key_down() const;
    void on_key_down(const on_key_down_func& on_key_down);

    using on_key_up_func = std::function<void (key)>;
    on_key_up_func on_key_up() const;
    void on_key_up(const on_key_up_func& on_key_up);

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

namespace detail { 
unsigned get_last_error();
} // namespace detail


void throw_system_error(const std::string& what, const unsigned error_code = detail::get_last_error());

using on_idle_func = std::function<void (void)>;
int run_message_loop(const on_idle_func& on_idle);

}

#endif