# Author: William Brorsson (aka s3cty, secty, sector)
# Date Created: Mars 26, 2025

# Specify minimum cmake version
cmake_minimum_required(VERSION 3.5)

# project name and version
project(breakanoid VERSION 0.1.0 LANGUAGES C)

# specify C++ standard
set(CMAKE_C_STANDARD 23)
# Without extensions False will set standard to gnu23 isntead of c23.
set(CMAKE_C_EXTENSIONS False)
set(CMAKE_C_STANDARD_REQUIRED True)

# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
# endif()
# message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")# "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_DEBUG "-ggdb3 -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Get the current Git branch
execute_process(
    COMMAND git branch --show-current
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the current Git commit hash (full)
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the current Git commit hash (short=7)
execute_process(
    COMMAND git rev-parse --short=7 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Configure version header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h)

# FROM: https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-cmake.md
# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# SET PATH TO DEPENDENCY DIRECTORIES HERE
set(SDL3_DIR                  "dep_repos/SDL")
set(cglm_DIR                  "dep_repos/cglm")
set(stb_DIR                   "dep_repos/stb")

# This requires the packages be found and avaiable
# In the case of Vulkan at least, the varaibles
# - Vulkan_LIBRARIES
# - Vulkan_INCLUDE_DIR
# are also set, I think?
find_package(Vulkan REQUIRED)

# Add executable
add_executable(breakanoid src/main.c src/vulkan/vulkan_engine.c src/SDL/SDL_backend.c src/util/deletion_queue.c)

# Add dependencies
add_subdirectory(${SDL3_DIR} EXCLUDE_FROM_ALL ${CMAKE_BINARY_DIR}/SDL3)
add_subdirectory(${cglm_DIR} ${CMAKE_BINARY_DIR}/cglm EXCLUDE_FROM_ALL)

target_include_directories(breakanoid SYSTEM PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(breakanoid SYSTEM PRIVATE ${cglm_DIR}/include)
target_include_directories(breakanoid SYSTEM PRIVATE ${stb_DIR})

# Include directory
target_include_directories(breakanoid PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)
# Link libraries
target_link_libraries(breakanoid PRIVATE m SDL3::SDL3 ${Vulkan_LIBRARIES})

# Set target compile options
target_compile_options(breakanoid PRIVATE 
    -pedantic 
    -Wall 
    -Wextra 
    -fmessage-length=0 
    -Wno-unused-variable 
    -Wfloat-equal 
    -Wundef 
    -Wshadow 
    -Wpointer-arith 
    -Wcast-align 
    # -Wstrict-overflow=5 
    -Wwrite-strings 
    # -Waggregate-return 
    -Wcast-qual 
    -Wswitch-enum 
    -Wconversion 
    -Wunreachable-code 
    -Wunused-macros 
    -Wduplicated-cond 
    -Wduplicated-branches
)

# Add compile definitions
if(WIN32)   
    add_compile_definitions(-DSYS_WINNT)
elseif(UNIX)
    add_compile_definitions(-DSYS_LINUX)
endif()
