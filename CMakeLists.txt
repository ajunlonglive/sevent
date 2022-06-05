cmake_minimum_required(VERSION 2.6)
project(sevent)

# 选项
option(BUILD_EXAMPLES "build examples" OFF)
option(BUILD_TESTS "build tests" OFF)
option(BUILD_RELEASE "build release" OFF)
option(ENABLE_OPENSSL "enable openssl" ON)

# 编译选项
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

if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

if(BUILD_RELEASE)
  set(CMAKE_BUILD_TYPE "Release")
else()
  set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")


# 目标文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

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
add_library(sevent_net ${SRC_NET})
target_link_libraries(sevent_net sevent_base)

# 依赖 ws2_32
if(WIN32)
    target_link_libraries(sevent_base ws2_32)
endif()

# 依赖 openssl
if(ENABLE_OPENSSL)
  find_package(OpenSSL REQUIRED)
  if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    aux_source_directory(${PROJECT_SOURCE_DIR}/sevent/net/ssl SRC_SSL)
    target_link_libraries(sevent_net ${OPENSSL_LIBRARIES} ${SRC_SSL})
  endif()
endif()

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

# set(EXE "httpsClient")
# add_executable(${EXE} ${PROJECT_SOURCE_DIR}/examples/https/httpsClient.cpp ${SRC_SSL})
# target_link_libraries(${EXE} sevent_net)
# target_link_libraries(${EXE} ${ssl} ${crypto})

