cmake_minimum_required(VERSION 3.10)

add_library(
  dle_graph
  SHARED
  agent.cpp
  edge.cpp
  errno.cpp
  )

add_library(
  dle_ghs 
  SHARED 
  ghs.cpp 
  ghs_msg.cpp
  errno.cpp
  )

add_library(
  dle_seque 
  SHARED 
  seque.cpp
  errno.cpp
  )

target_link_libraries(dle_ghs dle_seque dle_graph)

