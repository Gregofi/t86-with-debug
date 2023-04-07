cmake_minimum_required(VERSION 3.5)
project(t86)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOG_LEVEL=4")
set(CMAKE_CXX_FLAGS_SANITIZER "-Wall -g -DLOG_LEVEL=4 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -DLOG_LEVEL=2 -O2")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

add_subdirectory(common)
add_subdirectory(t86)
add_subdirectory(debugger)
add_subdirectory(t86-cli)
add_subdirectory(dbg-cli)
add_subdirectory(t86-parser)

# Some stupid cmake thing to silence FetchContent warnings
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)

# Libraries ----------
# fmt
message("-- Fetching fmt library")
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)
message("-- Fetching fmt library - done")

# argparse
message("-- Fetching argparse library")
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v2.9
)
FetchContent_MakeAvailable(argparse)
message("-- Fetching argparse library - done")
# Libraries END ------------

# benchmarks
add_subdirectory(benchmarks)

# testing
enable_testing()
add_subdirectory(tests)
