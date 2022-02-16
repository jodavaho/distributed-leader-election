cmake_minimum_required(VERSION 3.10)

project(GHS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS " -Wpedantic  -Wall -Werror ${CMAKE_CXX_FLAGS}")

include_directories(deps/doctest)
include_directories(include)

add_subdirectory(src/lib)
add_subdirectory(src/libtest)
add_subdirectory(src/test)
add_subdirectory(src/tools)

