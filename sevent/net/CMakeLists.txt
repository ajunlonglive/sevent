# set(SEVENT_NET_HEADERS 
#     Buffer.h
#     SocketsOps.h
# )
aux_source_directory(. SEVENT_NET_SRCS)
add_library(sevent_net STATIC ${SEVENT_NET_SRCS})
target_link_libraries(sevent_net sevent_base)

# file(GLOB SEVENT_NET_HEADERS "*.h")
# install(TARGETS sevent_net DESTINATION lib)
# install(FILES ${SEVENT_NET_HEADERS} DESTINATION include/sevent/net)

# if(SEVENT_BUILD_EXAMPLES)
#   add_subdirectory(tests)
# endif()