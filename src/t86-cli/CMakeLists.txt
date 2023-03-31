cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86-cli")

project(${PROJECT_NAME})
add_executable(t86-cli main.cpp)
target_link_libraries(t86-cli t86 common fmt::fmt argparse::argparse)
install(TARGETS t86-cli)
