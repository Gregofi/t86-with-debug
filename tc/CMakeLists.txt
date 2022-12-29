cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -pedantic -g -D__DEBUG__ -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory(tinyC)
add_subdirectory(fmt)
add_subdirectory(applications/ni-gen)

project(tiny)
