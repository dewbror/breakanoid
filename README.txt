Author: William Brorsson
Date Created: Mars 26, 2025

Breakanoid 0.1.0
----------------

Breakanoid is an open source, 2D game written in C99 using vulkan 1.0 and SDL3.

How to build with cmake
-----------------------

Run the clone_deps.bat/.sh script to clone dependencies into project directory. You must have the vulkan SDK installed.
Run the following commands in decending order.

  mkdir build
  cd build
  cmake ..
  cmake --build .

Dependencies
------------

  - Vulkan SDK https://vulkan.lunarg.com
  - SDL3       https://github.com/libsdl-org/SDL
  - CGLM       https://github.com/recp/cglm
  - stb_image  https://github.com/nothings/stb

TODO
----

  - Write scripts for running clang-format and clang-tidy.
  - Write scripts for creating zip/tarball.
  - Add descriptions to all function declerations and structs.

License
-------

The source code in this repository is licensed under the zlib License, see LICENSE. This license applies only to the source code files.

--- end of README ---
