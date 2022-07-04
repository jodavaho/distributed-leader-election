cmake_minimum_required(VERSION 3.03)

set (CMAKE_CXX_STANDARD 11)
find_library(nng nng REQUIRED)
find_library(nng inih REQUIRED)

set(GHS_DEMO_EXE_SRC
  ghs-demo-comms.cpp
  ghs-demo-inireader.cpp 
  ghs-demo-edgemetrics.cpp
  ghs-demo-clireader.cpp
  ghs-demo-msgutils.cpp
  )

set(GHS_DEMO_LIBS
  dle_ghs 
  dle_seque 
  dle_ghs_ext 
  nng 
  inih
  pthread
  )

set(GHS_DEMO_DOCTEST_SRC 
  ghs-demo-comms.cpp
  )


add_executable(ghs-demo ghs-demo.cpp ${GHS_DEMO_EXE_SRC})
target_link_libraries(ghs-demo ${GHS_DEMO_LIBS})

add_executable(ghs-demo-doctest 
  ghs-demo-doctest.cpp ${GHS_DEMO_DOCTEST_SRC} ${GHS_DEMO_EXE_SRC})
target_link_libraries(ghs-demo-doctest ${GHS_DEMO_LIBS})

add_test(ghs-demo-doctest ghs-demo-doctest)
