add_executable(socks4a socks4a.cpp)
target_link_libraries(socks4a sevent_net)

add_executable(tcprelay tcprelay.cpp)
target_link_libraries(tcprelay sevent_net)
