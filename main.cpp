#include <iostream>
#include <stdexcept>

#include "win32.h"

void on_idle()
{
}

int main()
{
    using namespace skirmish;

    try {
        d3d11_window w{};
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
