cmake_minimum_required(VERSION 3.15)
include(ExternalProject)

project(ghs)

OPTION(BUILD_DEMO "Build ghs-demo" OFF) 
OPTION(BUILD_DOCTEST "Build ghs-doctest" OFF) 
OPTION(BUILD_TOOLS "(DEPRECATED) CLI testing tools" OFF) 

set (CMAKE_CXX_STANDARD 11)

add_compile_options(-Wpedantic)
add_compile_options(-Wall)
add_compile_options(-Werror)

include_directories(include)
add_subdirectory(src/lib)

# Supporting library that doesn't need to be built for ROS / deployment
if (BUILD_TOOLS OR BUILD_DOCTEST OR BUILD_DEMO)
add_subdirectory(src/libtest)
endif (BUILD_TOOLS OR BUILD_DOCTEST OR BUILD_DEMO)

# Some CLI tools for testing specific graphs very quickly
if (BUILD_TOOLS)
  message("---- [BUILD_TOOLS=On] Building doctest, make sure doctest-dev is installed (see get_deps.sh)")
  add_subdirectory(src/tools)
else()
  message("---- [BUILD_TOOLS=Off] ")
endif(BUILD_TOOLS)

# The full doctest suite (requires doctest-dev)
if (BUILD_DOCTEST)
  message("---- [BUILD_DOCTEST=On] Building doctest, make sure doctest-dev is installed (see get_deps.sh)")
  add_subdirectory(src/ghs-tests)
else ()
  message("---- [BUILD_DOCTEST=Off]")
endif (BUILD_DOCTEST)

if (BUILD_DEMO)
  message("---- [BUILD_DEMO=On] Building demo, make sure nng-dev is installed (see get_deps.sh)")
  add_subdirectory(src/demo)
  if (ENABLE_COMPRESSION)
    message("------ [ENABLE_COMPRESSION=On] Building ghs-demo with compression, make sure ext/miniz is populated (see get_deps.sh)")
    add_subdirectory(${PROJECT_SOURCE_DIR}/ext/miniz)
    include_directories(${PROJECT_SOURCE_DIR}/ext/miniz)
  endif(ENABLE_COMPRESSION)
else ()
  message("---- [BUILD_DEMO=Off] ")
endif(BUILD_DEMO)
