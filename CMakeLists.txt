cmake_minimum_required(VERSION 3.10)

project(GHS)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS " -Wpedantic  -Wall -Werror ${CMAKE_CXX_FLAGS}")

include_directories(deps/doctest)

#add_executable(ghs-score src/ghs-score.cpp) 
#target_link_libraries(ghs-score ghs-alg )

add_executable(random-graph src/random-graph.cpp src/msg_printer.cpp)
add_executable(to-dot src/to-dot.cpp src/msg_printer.cpp)

# Tests
add_executable(test 
  src/tests.cpp 
  src/ghs.cpp 
  src/msg.cpp 
  src/msg_printer.cpp 
  src/ghs_printer.cpp 
  src/graph.cpp 
  src/seque.cpp
)
