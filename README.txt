Author: William Brorsson (sector, secty, sectyy, dewb)
Date Created: Mars 26, 2025                                  

Breakanoid

DESCRIPTION

    Creating a game called Breakanoid.
    
HOW TO BUILD CMAKE

    Run the clone_deps.bat/.sh scripts to clone dependencies into project directory.

    mkdir build
    cd build
    cmake ..
    cmake --build .

DEPENDENCIES

    Vulkan SDK    1.4.304.1 https://vulkan.lunarg.com
    SDL                     https://github.com/libsdl-org/SDL
    CGLM                     https://github.com/recp/cglm
    stb_image               https://github.com/nothings/stb
    VMA                     https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

TODO

    - Write scripts for running clang-format and clang-tidy.
    - Write scripts for creating zip/tarball.