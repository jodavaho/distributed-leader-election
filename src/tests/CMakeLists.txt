cmake_minimum_required(VERSION 3.10)

add_executable(ghs-doctest tests.cpp )

target_link_libraries(ghs-doctest ghs ghs_ext)

add_test(ghs-doctest ghs-doctest)

