add_executable(basis main.cpp)
set(CMAKE_CXX_VERSION 17)
target_include_directories(basis PRIVATE basis_obj)
target_link_libraries(basis basis_obj)