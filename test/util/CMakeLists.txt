add_executable(test_util test_stream.cpp ${CATCH_MAIN_CPP})
target_link_libraries(test_util skirmish_util)
add_test(test_util test_util)
