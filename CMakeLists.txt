cmake_minimum_required(VERSION 3.30)
project(basis)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "*.cpp")
add_library(basis_obj STATIC ${SOURCES})
set_target_properties(basis_obj PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
add_subdirectory(basis_tests)
add_subdirectory(basis_main)
