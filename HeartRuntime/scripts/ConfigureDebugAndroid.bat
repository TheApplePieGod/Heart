#!/bin/bash

cmake -S .. -B ../build -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake -G Ninja
