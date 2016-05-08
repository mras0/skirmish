cmake_minimum_required(VERSION 3.0)
project(skirmish)

if (MSVC)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

    # Reset flags
    set(CMAKE_CXX_FLAGS "/EHsc")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Od /D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /DNDEBUG")

    # Always include debug information
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /Zo")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221") # warning LNK4221: This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library

    add_definitions("/W4")
    #add_definitions("/wd4127") # C4127: conditional expression is constant
    #add_definitions("/wd4709") # C4709: comma operator within array index expression
    #add_definitions("/wd4592") # C4592: '???': symbol will be dynamically initialized (implementation limitation)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -std=c++1z -g")
    if (MINGW)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/mingw)
    endif()
    #TODO: Make sure boost filesystem is available
endif()

if (WIN32)
    add_definitions("-DWIN32 -D_SCL_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE")
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    # On windows use: cmake .. -TLLVM-vs2014 -G"Visual Studio 14 2015 Win64"
    add_definitions("-Wno-missing-braces -Wall -Wextra")
endif()

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure --build-config "$<CONFIGURATION>")

add_subdirectory(third_party)

add_subdirectory(src)
include_directories(src)
add_subdirectory(test)
add_subdirectory(skirmish)
