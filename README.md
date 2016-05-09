skirmish
========

Tested with: Microsoft Visual Studio 14 (2015) Update 2, Clang 3.8 and g++ 5.3.0 (from http://nuwen.net/mingw.html)

Requires: CMake, ...

To build:
    * Create build directory
    * Use CMake to generate project files: `cmake <sourcedir> <cmake-options>`
    * Build using either make (for g++ or msbuild/visual studio for MSVC/Clang)
    * Build the `check` target to run the unit tests
