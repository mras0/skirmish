add_library(skirmish_win32
    d3d11_renderer.cpp
    d3d11_renderer.h
    q3_player_render_obj.cpp
    q3_player_render_obj.h
    win32_main_window.cpp
    win32_main_window.h
    )
target_link_libraries(skirmish_win32 skirmish_md3)
target_link_libraries(skirmish_win32 d3d11)
if (MSVC)
    target_link_libraries(skirmish_win32 d3dcompiler)
else()
    target_link_libraries(skirmish_win32 d3dcompiler_47)
endif()