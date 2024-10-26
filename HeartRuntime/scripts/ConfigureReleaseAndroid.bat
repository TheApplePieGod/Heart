#!/bin/bash

cmake -S .. -B ../build -DCMAKE_BUILD_TYPE="Release" -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake -G Ninja
