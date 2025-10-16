file(GLOB TEST_SOURCES "*.cpp")
set(CMAKE_CXX_VERSION 17)
add_executable(basis_tests_run ${TEST_SOURCES})
target_link_libraries(basis_tests_run basis_obj)
