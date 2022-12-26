#!/bin/bash

cmake -S .. -B ../build -DCMAKE_BUILD_TYPE="Release" -DHEART_BUILD_EDITOR=0 -DHEART_BUILD_RUNTIME=1 -G Xcode
