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
test
You must have the vulkan SDK installed.

The cmake config will clone the project dependencies into the project directory by default, this includes cloneing SDL and building SDL as a part of the project. If you would rather use your installed version of SDL3, include the cmake flag "-DUSE_SYSTEM_SDL=ON" when configuring cmake.

Build release ("-O3 -DNDEBUG")
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build_release
cmake --build ./build_release 
```
Build debug ("-ggdb3 -O0")
```
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B ./build_debug
cmake --build ./build_debug
```
Build release with debug info ("-ggdb3 -O2")
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B ./build_relwithdebinfo
cmake --build ./build_relwithdebinfo
```
Build with Ninja Multi-Config
```
cmake -DCMAKE_DEFAULT_BUILD_TYPE=Debug -G "Ninja Multi-Config" -S . -B ./build
cmake --build ./build --config Release
cmake --build ./build --config Debug
cmake --build ./build --config RelWithDebInfo
```

License
-------

The source code in this repository is licensed under the zlib License, see LICENSE. This license applies only to the source code files.

--- end of README ---
