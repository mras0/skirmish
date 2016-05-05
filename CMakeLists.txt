cmake_minimum_required(VERSION 3.0)
project(skirmish)

if (NOT MSVC)
    message(FATAL_ERROR "Only MSVC supported for now.")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build configurations" FORCE)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure --build-config "$<CONFIGURATION>")

# Reset flags
set(CMAKE_CXX_FLAGS "/EHsc")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Od /D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /DNDEBUG")

# Always include debug information
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /Zo")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")

add_definitions("/W4")
#add_definitions("/wd4127") # C4127: conditional expression is constant
#add_definitions("/wd4709") # C4709: comma operator within array index expression
#add_definitions("/wd4592") # C4592: '???': symbol will be dynamically initialized (implementation limitation)

add_definitions("-DWIN32 -D_SCL_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE")

add_subdirectory(third_party)

add_subdirectory(src)
include_directories(src)
add_subdirectory(test)
add_subdirectory(skirmish)
