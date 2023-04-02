set(INSTALL_GTEST 0)
# GTEST
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
  unittests
  t86/parser_test.cpp
  t86/debug_test.cpp
  utils_test.cpp
  debugger/t86process_test.cpp
  debugger/native_test.cpp
  debugger/source_test.cpp
  debugger/expr_test.cpp
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

