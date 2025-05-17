# Author: William Brorsson <dewb@duck.com>
# Date Created: Mars 26, 2025

# Specify minimum cmake version
cmake_minimum_required(VERSION 3.5)

# project name and version
project(break VERSION 0.1.0 LANGUAGES C)

# specify C standard
set(CMAKE_C_STANDARD 99)

# Without extensions False will set standard to gnu99 instead of c99.
set(CMAKE_C_EXTENSIONS False)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set Debug as the default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    # message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
    message(STATUS "Vulkan Include Directory: ${Vulkan_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Could not find Vulkan!")
endif()

# FROM: https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-cmake.md
# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Check if cglm and stb exist, if either one doesnt then run the dependency clone script
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/cglm" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/external/stb" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/external/cmocka")
    message(STATUS "Running clone script...")
    if(WIN32)
        execute_process(
            COMMAND "${CMAKE_SOURCE_DIR}/scripts/clone_dependencies.bat" SDL_OFF
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
    else()
        execute_process(
            COMMAND "${CMAKE_SOURCE_DIR}/scripts/clone_dependencies.sh" SDL_OFF
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
    endif()
endif()

# Set cglm and stb directories
set(cglm_DIR "${CMAKE_SOURCE_DIR}/external/cglm")
set(stb_DIR  "${CMAKE_SOURCE_DIR}/external/stb")

# Enable shared libraries globally
set(BUILD_SHARED_LIBS ON)

set(SDL3_SHARED_ENABLED_BY_DEFAULT ON  CACHE BOOL "Enable SDL shared library")       # Ensure the shared library is enabled
set(SDL3_STATIC_ENABLED_BY_DEFAULT OFF CACHE BOOL "Disable SDL static library")      # Disable the static library
set(SDL3_TESTS                     OFF CACHE BOOL "Disable SDL test programs" FORCE) # Disable SDL test programs
set(SDL3_INSTALL                   OFF CACHE BOOL "Disable SDL instalation rules")   # Disable SDL installation rules 

# Disable unnecessary SDL3 features to reduce build size (dep_repos/SDL/CMakeLists.txt:250-274)
set(SDL_AUDIO_DEFAULT      OFF CACHE BOOL "Disable SDL audio support")
# set(SDL_VIDEO_DEFAULT      OFF CACHE BOOL "Disable SDL video support")
set(SDL_GPU_DEFAULT        OFF CACHE BOOL "Disable SDL GPU support")
set(SDL_RENDER_DEFAULT     OFF CACHE BOOL "Disable SDL render support")
set(SDL_CAMERA_DEFAULT     OFF CACHE BOOL "Disable SDL camera support")
set(SDL_JOYSTICK_DEFAULT   OFF CACHE BOOL "Disable SDL joystick support")
set(SDL_HAPTIC_DEFAULT     OFF CACHE BOOL "Disable SDL haptic support")
set(SDL_HIDAPI_DEFAULT     OFF CACHE BOOL "Disable SDL hidapi support")
set(SDL_POWER_DEFAULT      OFF CACHE BOOL "Disable SDL power support")
set(SDL_SENSOR_DEFAULT     OFF CACHE BOOL "Disable SDL sensor support")
set(SDL_DIALOG_DEFAULT     OFF CACHE BOOL "Disable SDL locale support")

# Add an option to toggle between system-installed and embedded SDL
option(USE_SYSTEM_SDL "Use system-installed SDL instead of building SDL as a subdirectory" OFF)

# Configure SDL
if(USE_SYSTEM_SDL)
    find_package(SDL3 QUIET SDL3-shared)
    if(NOT SDL3_FOUND)
        message(FATAL_ERROR "System-wide SDL3 installation not found. Please install SDL3 or disable USE_SYSTEM_SDL.")
    endif()
    set(SDL3_LIBRARIES SDL3::SDL3-shared)
    set(SDL3_INCLUDE_DIR ${SDL3_INCLUDE_DIRS})
else()
    # Check if external/SDL exists
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/SDL")
        # message(STATUS "SDL not found in external. Running clone script...")
        if(WIN32)
            execute_process(
                COMMAND "${CMAKE_SOURCE_DIR}/scripts/clone_dependencies.bat" SDL_ONLY
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            )
        else()
            execute_process(
                COMMAND "${CMAKE_SOURCE_DIR}/scripts/clone_dependencies.sh" SDL_ONLY
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            )
        endif()
        # Re-check if the clone was successful
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/SDL")
            message(FATAL_ERROR "SDL could not be cloned!")
        endif()
    else()
        # message(STATUS "SDL found in external")
    endif()
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/SDL)
    set(SDL3_LIBRARIES SDL3::SDL3-shared)
    set(SDL3_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/SDL/include)
endif()
# Necessary in order for SDL_test to not be built
set_property(TARGET SDL3_test PROPERTY EXCLUDE_FROM_ALL TRUE)
set_property(TARGET SDL3_test PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)

# Set this to siable building cmocka examples, CMOCKA_WITH_EXAMPLES does NOT work!
set(WITH_EXAMPLES OFF CACHE BOOL "Disable CMocka examples")
add_subdirectory(${CMAKE_SOURCE_DIR}/external/cmocka)

# Add external subdirectory
# add_subdirectory(external)

# Add source subdirectory
add_subdirectory(src)

# Add tests subdirectory (only on UNIX currently)
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS AND UNIX)
    add_subdirectory(tests)
endif()
