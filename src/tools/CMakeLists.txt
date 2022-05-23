add_executable(random-graph random-graph.cpp)
target_link_libraries(random-graph ghs_ext)

add_executable(to-dot to-dot.cpp )
target_link_libraries(to-dot  ghs_ext)

add_executable(ghs-score ghs-score.cpp)
target_link_libraries(ghs-score ghs_ext)

message("---- [tools/] warning, these tools are not well supported")
