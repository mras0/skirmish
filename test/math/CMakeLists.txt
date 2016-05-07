add_executable(test_math
    test_math.cpp
    test_3dmath.cpp
    matvecio.h
    ${CATCH_MAIN_CPP})
target_link_libraries(test_math skirmish_math)
add_test(test_math test_math)