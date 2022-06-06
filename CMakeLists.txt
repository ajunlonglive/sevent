cmake_minimum_required(VERSION 3.0)
project(sevent)

# 选项
option(BUILD_EXAMPLES "build examples" OFF)
option(BUILD_TESTS "build tests" OFF)
option(BUILD_RELEASE "build release" ON)
option(ENABLE_OPENSSL "enable openssl" OFF)

# 编译选项
if(BUILD_RELEASE)
  set(CMAKE_BUILD_TYPE "Release")
else()
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CXX_FLAGS
  -pthread
  -g
  -D_FILE_OFFSET_BITS=64
  -Wall
  -Wextra
  -Wconversion
  -Wno-unused-parameter
  -Wold-style-cast
  -Woverloaded-virtual
  -Wpointer-arith
  -Wwrite-strings
  -march=native
  -std=c++17
  #  -Wshadow
  #  -rdynamic
  )
  set(CMAKE_CXX_FLAGS_DEBUG "-O0")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
  if(CMAKE_BUILD_BITS EQUAL 32)
    list(APPEND CXX_FLAGS "-m32")
  endif()
endif()
# MSVC
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CXX_FLAGS /utf-8 /std:c++17 /EHsc)
  set(CMAKE_CXX_FLAGS_DEBUG "/Od")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 -DNDEBUG")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
# 打印编译选项
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "COMPILER_ID = " ${CMAKE_CXX_COMPILER_ID})
message(STATUS "BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})


# 依赖 openssl
if(ENABLE_OPENSSL)
  find_package(OpenSSL REQUIRED)
  if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net/ssl SRC_SSL)
  endif()
endif()

# 目标文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin2)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib2)

# 头文件
include_directories(${PROJECT_SOURCE_DIR})

# 源文件
aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net/poller SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net/poller/wepoll SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net/http SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net/http/http-parser SRC_NET)

# 库文件(目标文件)
add_library(sevent_base ${SRC_BASE})
add_library(sevent_net ${SRC_NET} ${SRC_SSL})
target_link_libraries(sevent_net sevent_base)

# 依赖 ws2_32
if(WIN32)
  target_link_libraries(sevent_base ws2_32)
endif()
# 依赖 openssl
target_link_libraries(sevent_net ${OPENSSL_LIBRARIES})

# 测试用例
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if(BUILD_TESTS)
  add_subdirectory(sevent/base/tests)
  add_subdirectory(sevent/net/tests)
  add_subdirectory(sevent/net/http/tests)
  if(ENABLE_OPENSSL)
    add_subdirectory(sevent/net/ssl/tests)
  endif()
endif()

# install 路径
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/sevent)
file(GLOB HEADERS_BASE ${PROJECT_SOURCE_DIR}/sevent/base/*.h)
file(GLOB HEADERS_NET ${PROJECT_SOURCE_DIR}/sevent/net/*.h)
file(GLOB HEADERS_NET_HTTP ${PROJECT_SOURCE_DIR}/sevent/net/http/*.h)
file(GLOB HEADERS_NET_SSL ${PROJECT_SOURCE_DIR}/sevent/net/ssl/*.h)
install(FILES ${HEADERS_BASE} DESTINATION ${INCLUDE_DIR}/base)
install(FILES ${HEADERS_NET} DESTINATION ${INCLUDE_DIR}/net)
install(FILES ${HEADERS_NET_HTTP} DESTINATION ${INCLUDE_DIR}/net/http)
install(FILES ${HEADERS_NET_SSL} DESTINATION ${INCLUDE_DIR}/net/ssl)