cmake_minimum_required(VERSION 3.5)

project(libcommon)

file(GLOB_RECURSE SRC "*.cpp" "*.h")
add_library(libcommon ${SRC})
