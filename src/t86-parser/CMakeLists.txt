cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86-parser")

project(${PROJECT_NAME})

add_library(${PROJECT_NAME} parser.cpp parser.h)
target_link_libraries(${PROJECT_NAME} t86 fmt::fmt)
