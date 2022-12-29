cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86-cli")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOGGER")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

project(${PROJECT_NAME})
add_executable(t86-cli main.cpp)
target_link_libraries(t86-cli t86 common)
