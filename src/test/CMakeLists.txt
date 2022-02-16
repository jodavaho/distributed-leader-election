cmake_minimum_required(VERSION 3.10)

add_executable(ghs-test tests.cpp )
target_link_libraries(ghs-test ghs_ext)

