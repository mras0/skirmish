#include "win32_main_window.h"
#include <string>
#include <system_error>
#include <cassert>
#include <windows.h>

namespace {

skirmish::key translate_key(WPARAM virtual_key) {
    if (virtual_key >= 'A' && virtual_key <= 'Z') return static_cast<skirmish::key>(virtual_key);

    switch (virtual_key) {
    case VK_ESCAPE: return skirmish::key::escape;
    case VK_LEFT:   return skirmish::key::left;
    case VK_RIGHT:  return skirmish::key::right;
    case VK_UP:     return skirmish::key::up;
    case VK_DOWN:   return skirmish::key::down;
    }
    return skirmish::key::one_past_max;
}

} // unnamed namespace

namespace skirmish {

static_assert(static_cast<int>(key::z) == 'Z', "");

namespace detail { 

unsigned get_last_error()
{
    return GetLastError();
}

} // namespace detail

void throw_system_error(const std::string& what, const unsigned error_code)
{
    assert(error_code != ERROR_SUCCESS);
    throw std::system_error(error_code, std::system_category(), what);
}

std::string wide_to_mb(const wchar_t* wide) {
    auto convert = [wide](char* mb, int mblen) { return WideCharToMultiByte(CP_ACP, 0, wide, -1, mb, mblen, nullptr, nullptr); };
    const int size = convert(nullptr, 0);
    if (!size) {
        throw_system_error("WideCharToMultiByte");
    }
    std::string result(size, '\0');
    if (!convert(&result[0], size)) {
        throw_system_error("WideCharToMultiByte");
    }
    assert(result.back() == '\0');
    result.pop_back();
    return result;
}

template<typename Derived>
class window_base {
public:
    explicit window_base() {
    }

    ~window_base() {
        if (hwnd_) {
            assert(IsWindow(hwnd_));
            DestroyWindow(hwnd_);
        }
        assert(!hwnd_);
    }

    void show() {
        assert(hwnd_);
        ShowWindow(hwnd_, SW_SHOWDEFAULT);
    }

    void close() {
        SendMessage(hwnd(), WM_CLOSE, 0, 0);
    }

    void set_text(const std::string& text) {
        // TODO: convert from ut8->utf16 and use SetWindowTextW
#ifndef NDEBUG
        for (auto c: text) assert(c >= 0 && c <= 127);
#endif
        SetWindowTextA(hwnd(), text.c_str());
    }

    HWND hwnd() {
        assert(hwnd_);
        return hwnd_;
    }

protected:
    void create() {
        assert(!hwnd_);
        register_class();

        if (!CreateWindow(Derived::class_name(), L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), this)) {
            throw_system_error("CreateWindow");
        }
        assert(hwnd_);
    }

private:
    HWND hwnd_ = nullptr;

    static void register_class() {
        WNDCLASS wc ={0,};
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = s_wndproc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszMenuName  = nullptr;
        wc.lpszClassName = Derived::class_name();

        if (!RegisterClass(&wc)) {
            // Todo: allow class to already be registered
            throw_system_error("RegisterClass");
        }
    }

    static LRESULT CALLBACK s_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        window_base* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto cs = *reinterpret_cast<const CREATESTRUCT*>(lparam);
            self = reinterpret_cast<window_base*>(cs.lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<window_base*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (msg == WM_NCDESTROY && self) {
            auto ret = DefWindowProc(hwnd, msg, wparam, lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            self->hwnd_ = nullptr;
            return ret;
        }

        return self ? self->wndproc(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        assert(hwnd_ == hwnd);
        switch (msg) {
        case WM_KEYDOWN:
            static_cast<Derived*>(this)->handle_key_down(wparam, lparam);
            return 0;
        case WM_KEYUP:
            static_cast<Derived*>(this)->handle_key_up(wparam, lparam);
            return 0;
        case WM_PAINT:
            static_cast<Derived*>(this)->handle_paint();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
};

class win32_main_window::impl : public window_base<win32_main_window::impl> {
public:
    explicit impl(unsigned width, unsigned height) {
        create();

        // Don't allow maximize/resize
        SetWindowLong(hwnd(), GWL_STYLE, GetWindowLong(hwnd(), GWL_STYLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX));

        // Adjust window size to match width/height
        RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        AdjustWindowRectEx(&rect, GetWindowLong(hwnd(), GWL_STYLE), FALSE, GetWindowLong(hwnd(), GWL_EXSTYLE));
        SetWindowPos(hwnd(), HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);

    }

    static const wchar_t* class_name() {
        return L"win32_main_window";
    }

    void handle_paint() {
        if (on_paint_) {
            on_paint_();
        } else {
            PAINTSTRUCT ps;
            if (BeginPaint(hwnd(), &ps)) {
                EndPaint(hwnd(), &ps);
            }
        }
    }

    void handle_key_down(WPARAM virtual_key, LPARAM extended) {
        auto k = translate_key(virtual_key);
        if (k == key::one_past_max) return;
        (void) extended;
        if (on_key_down_) {
            on_key_down_(k);
        }
    }

    void handle_key_up(WPARAM virtual_key, LPARAM extended) {
        auto k = translate_key(virtual_key);
        if (k == key::one_past_max) return;
        (void) extended;
        if (on_key_up_) {
            on_key_up_(k);
        }
    }

    on_paint_func on_paint() const {
        return on_paint_;
    }

    void on_paint(const on_paint_func& on_paint) {
        on_paint_ = on_paint;
    }

    on_key_down_func on_key_down() const {
        return on_key_down_;
    }

    void on_key_down(const on_key_down_func& on_key_down) {
        on_key_down_ = on_key_down;
    }

    on_key_up_func on_key_up() const {
        return on_key_up_;
    }

    void on_key_up(const on_key_up_func& on_key_up) {
        on_key_up_ = on_key_up;
    }
private:
    on_paint_func on_paint_;
    on_key_down_func on_key_down_;
    on_key_up_func on_key_up_;
};

win32_main_window::win32_main_window(unsigned width, unsigned height) : impl_(new impl{width, height})
{
}

win32_main_window::~win32_main_window() = default;

void win32_main_window::show()
{
    impl_->show();
}

void win32_main_window::close()
{
    impl_->close();
}

void win32_main_window::set_title(const std::string& text)
{
    impl_->set_text(text.c_str());
}

win32_main_window::native_handle_type win32_main_window::native_handle()
{
    return impl_->hwnd();
}

win32_main_window::on_paint_func win32_main_window::on_paint() const {
    return impl_->on_paint();
}

void win32_main_window::on_paint(const on_paint_func& on_paint)
{
    return impl_->on_paint(on_paint);
}

win32_main_window::on_key_down_func win32_main_window::on_key_down() const {
    return impl_->on_key_down();
}

void win32_main_window::on_key_down(const on_key_down_func& on_key_down)
{
    return impl_->on_key_down(on_key_down);
}

win32_main_window::on_key_up_func win32_main_window::on_key_up() const {
    return impl_->on_key_up();
}

void win32_main_window::on_key_up(const on_key_up_func& on_key_up)
{
    return impl_->on_key_up(on_key_up);
}

int run_message_loop(const on_idle_func& on_idle)
{
    assert(on_idle);

    for (;;) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return static_cast<int>(msg.wParam);
            }
     
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        on_idle();
    }
}


} // namespace skirmish