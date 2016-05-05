add_executable(test_util test_stream.cpp ${CATCH_MAIN_CPP})
set_source_files_properties(FILES test_stream.cpp PROPERTIES COMPILE_FLAGS "-DTEST_DATA_DIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/test_data\\\"")
target_link_libraries(test_util skirmish_util)
add_test(test_util test_util)
