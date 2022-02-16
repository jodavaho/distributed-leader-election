cmake_minimum_required(VERSION 3.10)

add_library(ghs_ext
  SHARED
  ghs_printer.cpp
  msg_printer.cpp
)

target_link_libraries(ghs_ext ghs ghs_seque)

