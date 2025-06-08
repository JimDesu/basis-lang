file(GLOB TEST_SOURCES "*.cpp")
add_executable(basis_tests_run ${TEST_SOURCES})
target_link_libraries(basis_tests_run basis_obj)
