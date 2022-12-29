cmake_minimum_required(VERSION 3.5)
project(t86)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(common)
add_subdirectory(t86)
add_subdirectory(t86-cli)

# # Testing
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
  parser_test
  tests/parser_test.cpp
)

target_link_libraries(
  parser_test
  gtest
  gtest_main
  t86
  common
)

include(GoogleTest)
gtest_discover_tests(parser_test)
