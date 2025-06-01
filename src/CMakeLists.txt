# Author: William Brorsson <dewb@duck.com>
# Date Created: Mars 26, 2025

# Specify minimum cmake version
cmake_minimum_required(VERSION 3.5)

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

message(STATUS "imgui_DIR = ${imgui_DIR}")
message(STATUS "cimgui_DIR = ${cimgui_DIR}")

add_library(imgui_lib STATIC
    ${imgui_DIR}/imgui.cpp
    ${imgui_DIR}/imgui_demo.cpp
    ${imgui_DIR}/imgui_draw.cpp
    ${imgui_DIR}/imgui_tables.cpp
    ${imgui_DIR}/imgui_widgets.cpp
    ${imgui_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_DIR}/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui_lib SYSTEM PUBLIC ${imgui_DIR} ${SDL3_INCLUDE_DIR}) 

add_library(cimgui_lib STATIC
    ${cimgui_DIR}/cimgui.cpp
)

target_include_directories(cimgui_lib SYSTEM PUBLIC ${cimgui_DIR})
target_link_libraries(cimgui_lib PUBLIC imgui_lib)

# GLOB all the source files
file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
# Remove main.c from the list, we dont want it in the static lib
list(REMOVE_ITEM SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
list(REMOVE_ITEM SRC ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.c)


# Add main code as a library
add_library(break_lib STATIC ${SRC})

# Add external include directories
target_include_directories(break_lib SYSTEM PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(break_lib SYSTEM PUBLIC ${SDL3_INCLUDE_DIR})
target_include_directories(break_lib SYSTEM PUBLIC ${cglm_DIR}/include)
target_include_directories(break_lib SYSTEM PUBLIC ${stb_DIR})
target_include_directories(break_lib SYSTEM PUBLIC ${cimgui_DIR})

# Include directory
target_include_directories(break_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(break_lib PUBLIC cimgui_lib m SDL3::SDL3-shared ${Vulkan_LIBRARIES})

# Compiler-specific flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # GCC
    target_compile_options(break_lib PUBLIC 
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
        # -Wswitch-enum 
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
        target_compile_options(break_lib PUBLIC
            -O0
            -ggdb3
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "DebWithAsan")
        target_compile_options(break_lib PUBLIC
            -O0
            -ggdb3
            -fsanitize=address,undefined
        )
        target_link_options(break_lib PUBLIC
            -fsanitize=address,undefined
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "DebWithCov")
        target_compile_options(break_lib PUBLIC
            -O0
            -ggdb3
            --coverage
        )
        target_link_options(break_lib PUBLIC
            --coverage
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        target_compile_options(break_lib PUBLIC
            -O2
            -ftree-vectorize
            -ftree-vectorizer-verbose=3
            -ggdb3
            -D_FORTIFY_SOURCE=2
            # -march=native
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(break_lib PUBLIC
            -O3
            -ftree-vectorize
            -ftree-vectorizer-verbose=3
            -DNDEBUG
            -D_FORTIFY_SOURCE=2
            # -march=native
        )
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Clang
    target_compile_options(break_lib PUBLIC 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wshadow 
        -Wundef
    )
    # Build type specific flags
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(break_lib PUBLIC
            -O0
            -g3
            -fsanitize=address,undefined
        )
        target_link_options(break_lib PUBLIC
            -fsanitize=address,undefined
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        target_compile_options(break_lib PUBLIC
            -O2
            -g3
            # -march=native
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(break_lib PUBLIC
            -O3
            -DNDEBUG
            # -march=native
        )
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    # MSVC
    target_compile_options(break_lib PUBLIC 
        /W4
    )
    # Build type specific flags
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(break PRIVATE
            /Zi
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        target_compile_options(break PRIVATE
            /O2
            /Zi
        )
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        target_compile_options(break PRIVATE
            /O3
            /DNDEBUG
        )
    endif()
endif()

# Add executables
add_executable(break ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
target_link_libraries(break PRIVATE break_lib)
