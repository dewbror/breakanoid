Author: William Brorsson (sector, secty, sectyy, dewb)
Date Created: Mars 26, 2025

Breakanoid 0.1.0
----------------

Breakanoid is an open source, 2D game written in C using vulkan 1.0 and SDL3.

I'm currently writing the game using features from C23, however, I am also trying to keep it compatible with C99.

Features used from C23
----------------------
    nullptr

How to build with cmake
-----------------------

Run the clone_deps.bat/.sh scripts to clone dependencies into project directory.
You must have the vulkan SDK installed.

    mkdir build
    cd build
    cmake ..
    cmake --build .

Dependencies
------------

    Vulkan SDK https://vulkan.lunarg.com
    SDL3       https://github.com/libsdl-org/SDL
    CGLM       https://github.com/recp/cglm
    stb_image  https://github.com/nothings/stb

TODO
----

    - Write scripts for running clang-format and clang-tidy.
    - Write scripts for creating zip/tarball.
    - Add descriptions to all function declerations and structs.

License
-------

This code is licensed under the zlib license.

--- end of README ---
