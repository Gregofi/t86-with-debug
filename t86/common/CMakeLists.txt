cmake_minimum_required(VERSION 3.5)

project(common)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOGGER")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

file(GLOB_RECURSE SRC "*.cpp")
add_library(common ${SRC})
