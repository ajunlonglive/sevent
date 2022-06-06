add_executable(hub hub.cpp hubCodec.cpp)
target_link_libraries(hub sevent_net)

add_library(sevent_pubsub pubsub.cpp hubCodec.cpp)
target_link_libraries(sevent_pubsub sevent_net)

add_executable(pub pub.cpp)
target_link_libraries(pub sevent_pubsub)

add_executable(sub sub.cpp)
target_link_libraries(sub sevent_pubsub)