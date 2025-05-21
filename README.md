Breakanoid 0.1.0
================

A Vulkan + SDL3 project.

TODO
----
- Fix "clone_dependencies.bat"
- Improve the "How to Build" section

Dependencies
------------

  - [Vulkan SDK](https://vulkan.lunarg.com)
  - [SDL3](https://github.com/libsdl-org/SDL) 3.2.14
  - [cglm](https://github.com/recp/cglm) 0.9.6
  - [stb_image](https://github.com/nothings/stb) 2.30
  - [cimgui](https://github.com/cimgui/cimgui) 1.53.1 (for development)
  - [cmocka](https://github.com/clibs/cmocka) 1.1.5 (for tests)

How to Build (with cmake)
-----------------------
You must have the vulkan SDK installed to build this project. Cmake will create a directory called `external` and fetch the remaining dependencies into it. SDL is added as a subdirectory and built along side Breakanoid. Include the flag `-DUSE_SYSTEM_SDL=ON` when configuring cmake to not fetch SDL and instead use your system installed SDL.

Build release
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build
cmake --build ./build 
```

License
-------

The source code in this repository is licensed under the zlib License, see [LICENSE.txt](https://github.com/dewbror/breakanoid/blob/master/LICENSE.txt). This license applies only to the source code files.

--- end of README ---
