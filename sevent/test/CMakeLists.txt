cmake_minimum_required(VERSION 2.6)
project(mytest CXX)

set(SEVENE_PATH "$ENV{HOME}/shares/sylar/test/sevent")
set(SEVENE_SOURCE_BASE_DIR ${SEVENE_PATH}/base)
set(SEVENE_SOURCE_NET_DIR ${SEVENE_PATH}/net)

#编译选项
set(CXX_FLAGS
 -pthread
 -g
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
# -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wwrite-strings
 -march=native
 -rdynamic
 -std=c++11
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
# set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

#头文件
include_directories(${SEVENE_SOURCE_BASE_DIR})
include_directories(${SEVENE_SOURCE_NET_DIR})
#源文件
aux_source_directory(${SEVENE_SOURCE_BASE_DIR} SRC_BASE)
aux_source_directory(${SEVENE_SOURCE_NET_DIR} SRC_NET)

#目标文件及路径
set(EXE "a.out")
add_executable(${EXE} testEventLoop.cpp ${SRC_BASE} ${SRC_NET})
set(EXECUTABLE_OUTPUT_PATH ../)
# target_link_libraries(${EXE} sevent_net)
# add_subdirectory(${SEVENE_SOURCE_BASE_DIR} base_dir)
# add_subdirectory(${SEVENE_SOURCE_NET_DIR} net_dir)