cmake_minimum_required(VERSION 3.10)

add_executable(test tests.cpp )

target_link_libraries(test ghs_ext)

