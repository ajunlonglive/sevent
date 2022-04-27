cmake_minimum_required(VERSION 2.6)

project(sevent CXX)

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
#  -Wshadow
 -Wwrite-strings
 -march=native
 -std=c++11
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_subdirectory(base)
add_subdirectory(net)

#目标文件路径
# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
# set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)