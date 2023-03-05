cmake_minimum_required(VERSION 3.5)
project(t86)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -DLOG_LEVEL=4")
set(CMAKE_CXX_FLAGS_SANITIZER "-Wall -g -DLOG_LEVEL=4 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -DLOG_LEVEL=1")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

add_subdirectory(common)
add_subdirectory(t86)
add_subdirectory(debugger)
add_subdirectory(t86-cli)
add_subdirectory(dbg-cli)
add_subdirectory(t86-parser)

# # Testing
enable_testing()

# Some stupid cmake bullsh*t to silence FetchContent warnings
cmake_policy(SET CMP0135 NEW)
include(FetchContent)

# Libraries ----------
# GTEST
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

# argparse
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)
# Libraries END ------------

add_executable(
  unittests
  tests/t86/parser_test.cpp
  tests/t86/debug_test.cpp
  tests/utils_test.cpp
  tests/debugger/t86process_test.cpp
  tests/debugger/native_test.cpp
  tests/debugger/source_test.cpp
)

target_link_libraries(
  unittests
  gtest
  gmock
  GTest::gtest_main
  fmt::fmt
  t86
  common
  t86-parser
  debugger
)

include(GoogleTest)
gtest_discover_tests(unittests)
