add_executable(pingpong_server pingpong_server.cpp)
target_link_libraries(pingpong_server sevent_net)

add_executable(pingpong_client pingpong_client.cpp)
target_link_libraries(pingpong_client sevent_net)