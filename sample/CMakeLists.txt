cmake_minimum_required(VERSION 3.0)
project(echo)

set(SEVENT_PATH "$ENV{HOME}/your sevent project")
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${SEVENT_PATH}/lib)
set(CMAKE_CXX_FLAGS "-Wall -pthread -std=c++17")

include_directories(${SEVENT_PATH}/include)

find_library(sevent_base sevent_base REQUIRED)
find_library(sevent_net sevent_net REQUIRED)

add_executable(echo echo.cpp)
target_link_libraries(echo ${sevent_net} ${sevent_base})
if(WIN32)
  target_link_libraries(echo ws2_32)
endif()