add_subdirectory(math)

add_library(skirmish_util perlin.cpp perlin.h tga.cpp tga.h util/stream.cpp util/stream.h)
target_link_libraries(skirmish_util zlibstatic)

add_library(skirmish_win32 win32_main_window.cpp win32_main_window.h d3d11_renderer.cpp d3d11_renderer.h)