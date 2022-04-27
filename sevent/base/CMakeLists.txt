# set(SEVENT_BASE_HEADERS 
#     CommonUtil.h
#     CountDownLatch.h
#     CurrentThread.h
#     FixedBuffer.h
#     LogAsynAppender.h
#     LogStream.h
#     Logger.h
#     Timestamp.h
#     noncopyable.h
# )
aux_source_directory(. SEVENT_BASE_SRCS)
add_library(sevent_base STATIC ${SEVENT_BASE_SRCS})
target_link_libraries(sevent_base pthread)

# file(GLOB SEVENT_BASE_HEADERS "*.h")
# install(TARGETS sevent_base DESTINATION lib)
# install(FILES ${SEVENT_BASE_HEADERS} DESTINATION include/sevent/base)

# if(SEVENT_BUILD_EXAMPLES)
#   add_subdirectory(tests)
# endif()