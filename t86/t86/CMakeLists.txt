cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOGGER")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

project(${PROJECT_NAME})
file(GLOB_RECURSE SRC "*.cpp" "*.h")
add_library(${PROJECT_NAME} ${SRC})
