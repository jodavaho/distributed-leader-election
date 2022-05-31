cmake_minimum_required(VERSION 3.15)

project(ghs)

OPTION(BUILD_DEMO "Build ghs-demo" OFF) 
OPTION(BUILD_DOCTEST "Build ghs-doctest" OFF) 
OPTION(BUILD_EXT "Build libghs_ext.so" OFF) 
OPTION(BUILD_TOOLS "(DEPRECATED) CLI testing tools" OFF) 
OPTION(ENABLE_ROS "Set up ROS and Catkin CMakeLists.txt" OFF) 
OPTION(BUILD_DOCS "Make doxygen documentation" On) 

set (CMAKE_CXX_STANDARD 11)

add_compile_options(-Wpedantic)
add_compile_options(-Wall)
add_compile_options(-Werror)

include_directories(include)
add_subdirectory(src/lib)

# Supporting library that doesn't need to be built for ROS / deployment
if (BUILD_TOOLS OR BUILD_DOCTEST OR BUILD_DEMO OR BUILD_EXT)
add_subdirectory(src/libtest)
endif (BUILD_TOOLS OR BUILD_DOCTEST OR BUILD_DEMO OR BUILD_EXT)

if (ENABLE_ROS)
  message("-- [ENABLE_ROS=On] Enabling ROS catkin integration")
  find_package(catkin REQUIRED)
  catkin_package(
    INCLUDE_DIRS include
    LIBRARIES ghs ghs_ext
  )
else()
  message("-- [ENABLE_ROS=Off]")
endif(ENABLE_ROS)

# Some CLI tools for testing specific graphs very quickly
if (BUILD_TOOLS)
  message("-- [BUILD_TOOLS=On] Building doctest, make sure doctest-dev is installed (see get_deps.sh)")
  add_subdirectory(src/tools)
else()
  message("-- [BUILD_TOOLS=Off] ")
endif(BUILD_TOOLS)

# The full doctest suite (requires doctest-dev)
if (BUILD_DOCTEST)
  message("---- [BUILD_DOCTEST=On] Building doctest, make sure doctest-dev is installed (see get_deps.sh)")
  add_subdirectory(src/tests)
else ()
  message("-- [BUILD_DOCTEST=Off]")
endif (BUILD_DOCTEST)

if (BUILD_DEMO)
  message("-- [BUILD_DEMO=On] Building demo, make sure nng-dev is installed (see get_deps.sh)")
  add_subdirectory(src/demo)
  if (ENABLE_COMPRESSION)
    message("-- [ENABLE_COMPRESSION=On] Building ghs-demo with compression, make sure ext/miniz is populated (see get_deps.sh)")
    add_subdirectory(${PROJECT_SOURCE_DIR}/ext/miniz)
    include_directories(${PROJECT_SOURCE_DIR}/ext/miniz)
  endif(ENABLE_COMPRESSION)
else ()
  message("-- [BUILD_DEMO=Off] ")
endif(BUILD_DEMO)

if (BUILD_DOCS)
  message("-- [BUILD_DOCS=On], make sure doxygen is installed ")
  find_package(Doxygen REQUIRED)
  message("-- [BUILD_DOCS] Doxygen version: ${DOXYGEN_VERSION}")
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_MAN YES)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")
  set(DOXYGEN_PROJECT_NAME "GHS")
  set(DOXYGEN_PROJECT_BRIEF "Leader Election Based on GHS Minimum Spanning Tree")
  set(DOXYGEN_EXAMPLE_PATH ${PROJECT_SOURCE_DIR}/src/demo)
  doxygen_add_docs(
    doxygen
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include/ghs
    ${PROJECT_SOURCE_DIR}/include/seque
    ${PROJECT_SOURCE_DIR}/src/tests/
    ${PROJECT_SOURCE_DIR}/src/demo/
    ${PROJECT_SOURCE_DIR}/src/lib/
    ${PROJECT_SOURCE_DIR}/src/libtest/
    COMMENT "Generate man pages"
    )
  
else()
  message("-- [BUILD_DOCS=Off] ")
endif (BUILD_DOCS)
