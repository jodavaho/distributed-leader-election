cmake_minimum_required(VERSION 3.10)

add_library(dle_ghs_ext
  SHARED
  ghs_printer.cpp
  msg_printer.cpp
)

target_link_libraries(dle_ghs_ext dle_ghs dle_seque)

