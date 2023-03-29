cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86-parser")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOG_LEVEL=4")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -DLOG_LEVEL=1")

project(${PROJECT_NAME})

add_library(${PROJECT_NAME} parser.cpp parser.h)
target_link_libraries(${PROJECT_NAME} t86 fmt::fmt)
