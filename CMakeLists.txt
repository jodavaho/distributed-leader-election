cmake_minimum_required(VERSION 3.10)

project(GHS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS -Wall)

include_directories(deps/doctest)

add_library(ghs-alg src/ghs.cpp src/msg.cpp )

add_executable(ghs-score src/ghs-score.cpp) 
target_link_libraries(ghs-score ghs-alg)

add_executable(random-graph src/random-graph.cpp)
add_executable(stleader src/stleader.cpp src/msg.cpp)
add_executable(to-dot src/to-dot.cpp)

# Tests
add_executable(test src/tests.cpp src/ghs.cpp src/msg.cpp)
#target_link_libraries(test ghs-alg)
