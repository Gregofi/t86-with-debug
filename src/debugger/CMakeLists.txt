cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "debugger")

project(${PROJECT_NAME})
file(GLOB_RECURSE SRC "*.cpp" "*.c" "*.h")

add_library(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} t86 common t86-parser fmt::fmt argparse::argparse)
