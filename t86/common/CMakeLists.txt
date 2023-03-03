cmake_minimum_required(VERSION 3.5)

project(common)
include(FetchContent)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOGGER")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

include_directories(${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE SRC "*.cpp")
add_library(common ${SRC})
target_link_libraries(common fmt t86)
