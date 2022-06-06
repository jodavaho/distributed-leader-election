cmake_minimum_required(VERSION 3.03)

set (CMAKE_CXX_STANDARD 11)
find_library(nng nng REQUIRED)
find_library(nng inih REQUIRED)
OPTION(ENABLE_COMPRESSION "use miniz in ghs-demo" OFF) 

# Enable (very experimental) compression in ghs-demo
if (ENABLE_COMPRESSION)

  set(GHS_DEMO_EXE_SRC
    ghs-demo-comms.cpp
    ghs-demo-inireader.cpp 
    ghs-demo-edgemetrics.cpp
    ghs-demo-clireader.cpp
    ghs-demo-msgutils-z.cpp
  )

  set(GHS_DEMO_LIBS
    ghs 
    ghs_seque 
    ghs_ext 
    nng 
    inih
    pthread
    miniz
  )

  set(GHS_DEMO_DOCTEST_SRC 
    ghs-demo-comms.cpp
    ghs-demo-doctest-miniz.cpp 
    )

else()

  set(GHS_DEMO_EXE_SRC
    ghs-demo-comms.cpp
    ghs-demo-inireader.cpp 
    ghs-demo-edgemetrics.cpp
    ghs-demo-clireader.cpp
    ghs-demo-msgutils.cpp
  )

  set(GHS_DEMO_LIBS
    ghs 
    ghs_seque 
    ghs_ext 
    nng 
    inih
    pthread
  )

  set(GHS_DEMO_DOCTEST_SRC 
    ghs-demo-comms.cpp
    )

endif (ENABLE_COMPRESSION)

add_executable(ghs-demo ghs-demo.cpp ${GHS_DEMO_EXE_SRC})
target_link_libraries(ghs-demo ${GHS_DEMO_LIBS})

add_executable(ghs-demo-doctest 
  ghs-demo-doctest.cpp ${GHS_DEMO_DOCTEST_SRC} ${GHS_DEMO_EXE_SRC})
target_link_libraries(ghs-demo-doctest ${GHS_DEMO_LIBS})

add_test(ghs-demo-doctest ghs-demo-doctest)
