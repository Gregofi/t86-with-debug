cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME "ni-gen")

project(${PROJECT_NAME})

file(GLOB_RECURSE SRC "*.cpp" "*.h")
add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} libtinyc libcommon fmt)

