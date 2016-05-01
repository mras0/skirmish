add_library(skirmish_math vec.cpp vec.h mat.cpp mat.h)

add_library(skirmish_util perlin.cpp perlin.h tga.cpp tga.h)

add_executable(skirmish main.cpp skirmish.h win32_main_window.cpp win32_main_window.h d3d11_renderer.cpp d3d11_renderer.h)
target_link_libraries(skirmish skirmish_math skirmish_util)
