add_executable(HttpParser_test HttpParser_test.cpp)
target_link_libraries(HttpParser_test sevent_net)

add_executable(httpBuildString_test httpBuildString_test.cpp)
target_link_libraries(httpBuildString_test sevent_net)

add_executable(HttpReqCodecClient_test HttpReqCodecClient_test.cpp)
target_link_libraries(HttpReqCodecClient_test sevent_net)

add_executable(HttpRequestCodec_test HttpRequestCodec_test.cpp)
target_link_libraries(HttpRequestCodec_test sevent_net)

add_executable(HttpResCodecClient_test HttpResCodecClient_test.cpp)
target_link_libraries(HttpResCodecClient_test sevent_net)

add_executable(HttpResponseCodec_test HttpResponseCodec_test.cpp)
target_link_libraries(HttpResponseCodec_test sevent_net)

add_executable(httpServer_test httpServer_test.cpp)
target_link_libraries(httpServer_test sevent_net)