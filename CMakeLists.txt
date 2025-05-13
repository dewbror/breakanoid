# Author: William Brorsson (aka s3cty, secty, sector)
# Date Created: Mars 26, 2025

# Specify minimum cmake version
cmake_minimum_required(VERSION 3.5)

# project name and version
project(breakanoid VERSION 0.1.0 LANGUAGES C)

# specify C standard
set(CMAKE_C_STANDARD 99)

# Without extensions False will set standard to gnu99 instead of c99.
set(CMAKE_C_EXTENSIONS False)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
configure_file(${CMAKE_SOURCE_DIR}/src/version.h.in ${CMAKE_SOURCE_DIR}/src/version.h)

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
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/cglm" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")
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
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
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
    add_subdirectory(external/SDL)
    set(SDL3_LIBRARIES SDL3::SDL3-shared)
    set(SDL3_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/SDL/include)
endif()
# Necessary in order for SDL_test to not be built
set_property(TARGET SDL3_test PROPERTY EXCLUDE_FROM_ALL TRUE)
set_property(TARGET SDL3_test PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)

# Add executables
add_executable(breakanoid src/main.c src/logger.c src/vulkan/vulkan_engine.c src/SDL/SDL_backend.c src/util/deletion_queue.c src/game/game.c)

# Add external include directories
target_include_directories(breakanoid SYSTEM PRIVATE ${Vulkan_INCLUDE_DIR})
target_include_directories(breakanoid SYSTEM PRIVATE ${SDL3_INCLUDE_DIR})
target_include_directories(breakanoid SYSTEM PRIVATE ${cglm_DIR}/include)
target_include_directories(breakanoid SYSTEM PRIVATE ${stb_DIR})

# Include directory
target_include_directories(breakanoid PRIVATE ${CMAKE_SOURCE_DIR}/src/)

# Link libraries
target_link_libraries(breakanoid PRIVATE m SDL3::SDL3-shared ${Vulkan_LIBRARIES})

# Compiler-specific flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # GCC
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
        -Wwrite-strings 
        -Wcast-qual 
        -Wswitch-enum 
        -Wconversion 
        -Wunreachable-code 
        -Wunused-macros 
        -Wduplicated-cond 
        -Wduplicated-branches
        -Wvla
        -Wvector-operation-performance
        -Wsuggest-attribute=pure
        -Wsuggest-attribute=const
        -Wsuggest-attribute=noreturn
        -Wmissing-noreturn
        -Wsuggest-attribute=malloc
        -Wsuggest-attribute=format
        -Wmissing-format-attribute
        -Wsuggest-attribute=cold
        -fstack-protector-strong
    )
    # Build type specific flags
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(breakanoid PRIVATE
            -O0
            -ggdb3
            -fsanitize=address,undefined
        )
        target_link_options(breakanoid PRIVATE
            -fsanitize=address,undefined
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        target_compile_options(breakanoid PRIVATE
            -O2
            -ggdb3
            -D_FORTIFY_SOURCE=2
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(breakanoid PRIVATE
            -O3
            -DNDEBUG
            -D_FORTIFY_SOURCE=2
        )
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Clang
    target_compile_options(my_program PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wshadow 
        -Wundef
    )
    # Build type specific flags
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(breakanoid PRIVATE
            -O0
            -g3
            -fsanitize=address,undefined
        )
        target_link_options(breakanoid PRIVATE
            -fsanitize=address,undefined
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        target_compile_options(breakanoid PRIVATE
            -O2
            -g3
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(breakanoid PRIVATE
            -O3
            -DNDEBUG
        )
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    # MSVC
    target_compile_options(breakanoid PRIVATE 
        /W4
    )
    # Build type specific flags
    # if(CMAKE_BUILD_TYPE MATCHES "Debug")
    #     target_compile_options(breakanoid PRIVATE
    #         /Zi
    #     )
    # elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    #     target_compile_options(breakanoid PRIVATE
    #         /O2
    #         /Zi
    #     )
    # elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    #     target_compile_options(breakanoid PRIVATE
    #         /O3
    #         /DNDEBUG
    #     )
    # endif()
endif()

# target_compile_options(breakanoid PRIVATE
#     $<$<CONFIG:Debug>:-ggdb3>
#     $<$<CONFIG:Debug>:-O0>
#     $<$<CONFIG:Debug>:-fsanitize=address,undefined>
#     
#     $<$<CONFIG:Release>:-O3>
#     $<$<CONFIG:Release>:-DNDEBUG>
#     $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=2>
# 
#     $<$<CONFIG:RelWithDebInfo>:-ggdb3>
#     $<$<CONFIG:RelWithDebInfo>:-O2>
#     $<$<CONFIG:RelWithDebInfo>:-D_FORTIFY_SOURCE=2>
# 
#     -pedantic 
#     -Wall 
#     -Wextra 
#     -fmessage-length=0 
#     -Wno-unused-variable 
#     -Wfloat-equal 
#     -Wundef 
#     -Wshadow 
#     -Wpointer-arith 
#     -Wcast-align 
#     # -Wstrict-overflow=5 
#     -Wwrite-strings 
#     # -Waggregate-return 
#     -Wcast-qual 
#     -Wswitch-enum 
#     -Wconversion 
#     -Wunreachable-code 
#     -Wunused-macros 
#     -Wduplicated-cond 
#     -Wduplicated-branches
#     -Wvla
#     -Wvector-operation-performance
#     -Wsuggest-attribute=pure
#     -Wsuggest-attribute=const
#     -Wsuggest-attribute=noreturn
#     -Wmissing-noreturn
#     -Wsuggest-attribute=malloc
#     # -Wsuggest-attribute=returns_nonnull
#     -Wsuggest-attribute=format
#     -Wmissing-format-attribute
#     -Wsuggest-attribute=cold
#     -fstack-protector-strong
# )
# 
# target_link_options(breakanoid PRIVATE
#     $<$<CONFIG:Debug>:-fsanitize=address,undefined>  # Add ASan and UBSan for Debug builds
# )
