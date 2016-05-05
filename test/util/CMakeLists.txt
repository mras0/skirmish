add_definitions("-DTEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test_data\"")
add_executable(test_util test_stream.cpp test_zip.cpp ${CATCH_MAIN_CPP})
target_link_libraries(test_util skirmish_util)
add_test(test_util test_util)