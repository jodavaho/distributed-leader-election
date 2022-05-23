cmake_minimum_required(VERSION 3.10)

add_library(ghs 
  SHARED 
  ghs.cpp 
  graph.cpp 
  msg.cpp)

add_library(
  ghs_seque 
  SHARED 
  seque.cpp)

target_link_libraries(ghs ghs_seque)

