add_library(skirmish_util
    array_view.cpp
    array_view.h
    deflate_stream.cpp
    deflate_stream.h
    file_stream.cpp
    file_stream.h
    file_system.cpp
    file_system.h
    path.cpp
    path.h
    perlin.cpp
    perlin.h
    stream.cpp
    stream.h
    text.cpp
    text.h
    tga.cpp
    tga.h
    zip.cpp
    zip.h
    zip_internals.cpp
    zip_internals.h
    )

target_link_libraries(skirmish_util zlibstatic)
target_include_directories(skirmish_util PRIVATE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
