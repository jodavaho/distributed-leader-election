cmake_minimum_required(VERSION 3.10)

add_library(
  ghs 
  SHARED 
  ghs.cpp 
  agent.cpp
  edge.cpp
  msg.cpp
  errno.cpp
  )

add_library(
  ghs_seque 
  SHARED 
  seque.cpp
  errno.cpp
  )

target_link_libraries(ghs ghs_seque)

