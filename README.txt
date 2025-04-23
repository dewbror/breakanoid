Author: William Brorsson (sector, secty, sectyy, dewb)
Date Created: Mars 26, 2025                                  

Breakanoid 0.1.0

DESCRIPTION

    Breakanoid is an open source, 2D game written in C using vulkan 1.0 and SDL3. This is also a bit of a playground currently.

I'm currently writing the game using features from C23, however, I am also trying to keep it compatible down to C99, see src/types.h. Why C99? Because the vulkan C API, cglm and stb_image.h are all written in C99 and it's fun to try and maintain compatability down to C99. SDL3 is written in ANSI C.

Features used from C23
    nullptr
    
HOW TO BUILD CMAKE

    Run the clone_deps.bat/.sh scripts to clone dependencies into project directory.

    mkdir build
    cd build
    cmake ..
    cmake --build .

DEPENDENCIES

    Vulkan SDK 1.4.304.1 https://vulkan.lunarg.com
    SDL                  https://github.com/libsdl-org/SDL
    CGLM                 https://github.com/recp/cglm
    stb_image            https://github.com/nothings/stb

TODO

    - Write scripts for running clang-format and clang-tidy.
    - Write scripts for creating zip/tarball.