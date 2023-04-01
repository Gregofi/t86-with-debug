cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "bench")

project(${PROJECT_NAME})

add_executable(t86_bench t86_bench.cpp)
target_link_libraries(t86_bench t86 common fmt::fmt)

add_executable(native_bench native_bench.cpp)
target_link_libraries(native_bench t86 common fmt::fmt debugger)
