cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "debugger")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOG_LEVEL=4")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -DLOG_LEVEL=1")

project(${PROJECT_NAME})
file(GLOB_RECURSE SRC "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} fmt argparse)
