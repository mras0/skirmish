add_subdirectory(math)

add_library(skirmish_util util/array_view.cpp util/array_view.h perlin.cpp perlin.h tga.cpp tga.h util/stream.cpp util/stream.h util/deflate_stream.cpp util/deflate_stream.h)
target_link_libraries(skirmish_util zlibstatic)

add_library(skirmish_win32 win32_main_window.cpp win32_main_window.h d3d11_renderer.cpp d3d11_renderer.h)
