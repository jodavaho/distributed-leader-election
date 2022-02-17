cmake_minimum_required(VERSION 3.0.2)

project(ghs)
add_compile_options(-std=c++11)

add_compile_options(-Wpedantic)
add_compile_options(-Wall)
add_compile_options(-Werror)

include_directories(include)

find_package(catkin REQUIRED)
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ghs ghs_ext
)


add_subdirectory(src/lib)
add_subdirectory(src/libtest)
add_subdirectory(src/ghs-tests)
add_subdirectory(src/tools)

