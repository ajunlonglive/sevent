add_executable(simple_discard discard.cpp)
target_link_libraries(simple_discard sevent_net)

add_executable(simple_daytime daytime.cpp)
target_link_libraries(simple_daytime sevent_net)

add_executable(simple_time time.cpp)
target_link_libraries(simple_time sevent_net)

add_executable(simple_timeclient timeclient.cpp)
target_link_libraries(simple_timeclient sevent_net)

add_executable(simple_echo echo.cpp)
target_link_libraries(simple_echo sevent_net)

add_executable(simple_chargen chargen.cpp)
target_link_libraries(simple_chargen sevent_net)

add_executable(simple_allinone allinone.cpp)
target_link_libraries(simple_allinone sevent_net)