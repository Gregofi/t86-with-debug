cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86-cli")

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOG_LEVEL=4")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -DLOG_LEVEL=1")

# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

project(${PROJECT_NAME})
add_executable(t86-cli main.cpp)
target_link_libraries(t86-cli t86 common fmt::fmt argparse)
