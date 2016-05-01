add_subdirectory(math)

add_library(skirmish_util perlin.cpp perlin.h tga.cpp tga.h)

add_library(skirmish_win32 win32_main_window.cpp win32_main_window.h d3d11_renderer.cpp d3d11_renderer.h)
