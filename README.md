Breakanoid 0.1.0
================

Breakanoid is an open source, 2D game written in C99 using vulkan 1.0 and SDL3.

TODO
----
- Fix "clone_dependencies.bat"
- Improve the "How to Build" section

Dependencies
------------

  - [Vulkan SDK](https://vulkan.lunarg.com)
  - [SDL3](https://github.com/libsdl-org/SDL)
  - [cglm](https://github.com/recp/cglm)
  - [stb_image](https://github.com/nothings/stb)

How to Build (with cmake)
-----------------------
You must have the vulkan SDK installed.

Cmake will fetch the project dependencies (except for the vulkan SDK) into the project directory. SDL is added as a subdirectory and built along side Breakanoid. Include the flag "-DUSE_SYSTEM_SDL=ON" when configuring cmake to not fetch SDL and instead use the system installed SDL.

Build release
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build_release
cmake --build ./build_release 
```
Build debug
```
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B ./build_debug
cmake --build ./build_debug
```

License
-------

The source code in this repository is licensed under the zlib License, see [LICENSE.txt](https://github.com/dewbror/breakanoid/blob/master/LICENSE.txt). This license applies only to the source code files.

--- end of README ---
