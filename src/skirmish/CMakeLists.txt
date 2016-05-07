add_subdirectory(math)
add_subdirectory(util)
add_subdirectory(md3)
add_library(skirmish_win32 win32_main_window.cpp win32_main_window.h d3d11_renderer.cpp d3d11_renderer.h)
