cmake_minimum_required(VERSION 3.0.2)

project(ghs)
add_compile_options(-std=c++11)
find_package(catkin REQUIRED)
set(CMAKE_CXX_FLAGS " -Wpedantic  -Wall -Werror ${CMAKE_CXX_FLAGS}")

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ghs ghs_ext
)

include_directories(deps/doctest)
include_directories(include)

add_subdirectory(src/lib)
add_subdirectory(src/libtest)
add_subdirectory(src/test)
add_subdirectory(src/tools)

