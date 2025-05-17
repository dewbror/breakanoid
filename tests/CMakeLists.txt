# Author: William Brorsson <dewb@duck.com>
# Date Created: May 17, 2025

# Specify minimum cmake version
cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GLOB all the test source files
file(GLOB_RECURSE TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

# set(TEST_SRC
#     test_main.c
#     unit/test_logger.c
# )

# Add executables
add_executable(break_tests ${TEST_SRC})

# Include directory
target_include_directories(break_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(break_tests PRIVATE break_lib cmocka)
