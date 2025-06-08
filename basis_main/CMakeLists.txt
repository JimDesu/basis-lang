add_executable(basis main.cpp)
target_include_directories(basis PRIVATE basis_obj)
target_link_libraries(basis basis_obj)