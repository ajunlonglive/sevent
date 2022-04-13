cmake_minimum_required(VERSION 2.6)
project(mytest CXX)

set(SEVENE_PATH "$ENV{HOME}/shares/sylar/test/src")
set(SEVENE_SOURCE_DIR ${SEVENE_PATH}/base)

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
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
# set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

#源文件
aux_source_directory(${SEVENE_SOURCE_DIR} SRC_LIST)

#目标文件及路径
set(EXE "a.out")
add_executable(${EXE} testepoll.cpp ${SRC_LIST})
set(EXECUTABLE_OUTPUT_PATH ../)