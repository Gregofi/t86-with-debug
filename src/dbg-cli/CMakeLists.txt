cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "dbg-cli")

project(${PROJECT_NAME})
file(GLOB_RECURSE SRC "*.cpp" "*.c" "*.h")

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} t86 common t86-parser debugger fmt::fmt argparse::argparse)
install(TARGETS ${PROJECT_NAME})
