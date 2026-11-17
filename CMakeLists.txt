cmake_minimum_required(VERSION 3.15)
project(c-functional VERSION 0.1.0 LANGUAGES C)

# Strict ISO C99, no compiler extensions: the library must compile as
# plain C99 everywhere, including MSVC (which CMake maps to /std:c11,
# since MSVC has no C99 mode).
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Header-only library. Consume it with add_subdirectory() or
# FetchContent, then link c-functional to your target.
add_library(c-functional INTERFACE)
target_include_directories(c-functional INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Tests. One executable per test program, registered with CTest.
option(CF_BUILD_TESTS "Build the c-functional tests" ON)
if(CF_BUILD_TESTS)
  enable_testing()

  if(MSVC)
    add_compile_options(/W4 /WX)
  else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
  endif()

  add_executable(test_cf tests/test_cf.c)
  target_link_libraries(test_cf PRIVATE c-functional)
  add_test(NAME test_cf COMMAND test_cf)

  add_executable(test_multi tests/test_multi_a.c tests/test_multi_b.c)
  target_link_libraries(test_multi PRIVATE c-functional)
  add_test(NAME test_multi COMMAND test_multi)

  add_executable(test_unused tests/test_unused.c)
  target_link_libraries(test_unused PRIVATE c-functional)
  add_test(NAME test_unused COMMAND test_unused)

  add_executable(quickstart examples/quickstart.c)
  target_link_libraries(quickstart PRIVATE c-functional)
  add_test(NAME quickstart COMMAND quickstart)
endif()
