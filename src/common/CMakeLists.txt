cmake_minimum_required(VERSION 3.5)

project(common)
include(FetchContent)

include_directories(${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE SRC "*.cpp" "*.h")
add_library(common ${SRC})
target_link_libraries(common fmt::fmt)
