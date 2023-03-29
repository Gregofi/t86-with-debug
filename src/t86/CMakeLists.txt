
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_NAME "t86")

project(${PROJECT_NAME})
file(GLOB_RECURSE SRC "*.cpp" "*.h")

add_library(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} fmt::fmt t86-parser)
