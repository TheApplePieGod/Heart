#!/bin/bash
cmake -S .. -B ../build -DCMAKE_BUILD_TYPE="Debug" -DHEART_BUILD_EDITOR=1 -DHEART_BUILD_RUNTIME=0 -G Ninja
