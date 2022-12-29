cmake_minimum_required(VERSION 3.5)

project(fmt)

file(GLOB_RECURSE SRC "*.cc" "*.h")
add_library(fmt ${SRC})
