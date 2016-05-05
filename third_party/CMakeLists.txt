if (MSVC)
    add_definitions(/wd4131) # warning C4131: 'FUNCTION': uses old-style declarator
    add_definitions(/wd4244) # warning C4244: '=': conversion from 'unsigned int' to 'ush', possible loss of data
    add_definitions(/wd4127) # warning C4127: conditional expression is constant
endif()

set(SKIP_INSTALL_ALL TRUE)
add_subdirectory(zlib)
