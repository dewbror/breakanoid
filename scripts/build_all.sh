#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=Debug -S . -B ./build_debug
cmake --build ./build_debug
cmake -DCMAKE_BUILD_TYPE=DebWithCov -S . -B ./build_debwithcov
cmake --build ./build_debwithcov
cmake -DCMAKE_BUILD_TYPE=Release -S . -B ./build_release
cmake --build ./build_release
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B ./build_relwithdebinfo
cmake --build ./build_relwithdebinfo
