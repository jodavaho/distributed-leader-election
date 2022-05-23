cmake_minimum_required(VERSION 3.03)

set (CMAKE_CXX_STANDARD 11)
find_library(nng nng REQUIRED)
find_library(nng inih REQUIRED)
OPTION(ENABLE_COMPRESSION "use miniz in ghs-demo" OFF) 

# Enable (very experimental) compression in ghs-demo
if (ENABLE_COMPRESSION)

  set(GHS_DEMO_SRC
    ghs-demo.cpp 
    ghs-demo-comms.cpp
    ghs-demo-inireader.cpp 
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
    ghs-demo-doctest.cpp 
    ghs-demo-doctest-miniz.cpp 
    )


else()
  set(GHS_DEMO_SRC
    ghs-demo.cpp 
    ghs-demo-comms.cpp
    ghs-demo-inireader.cpp 
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

  set(GHS_DEMO_DOCTEST_SRC ghs-demo-doctest.cpp )

endif (ENABLE_COMPRESSION)

add_executable(ghs-demo ${GHS_DEMO_SRC})
target_link_libraries(ghs-demo ${GHS_DEMO_LIBS})

add_executable(ghs-demo-doctest ${GHS_DEMO_DOCTEST_SRC})
target_link_libraries(ghs-demo-doctest ${GHS_DEMO_LIBS})
