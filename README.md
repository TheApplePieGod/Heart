![Logo](logo.png)

# Heart

Heart is a cross-platform C++ 3D game engine. It can be used to create games using the editor or as a powerful rendering API library.

Some style and internal design choices for the engine were inspired by [TheCherno](https://www.youtube.com/user/TheChernoProject) (check him out!)

# Table of Contents

- [Introduction](#Introduction)
- [Getting Started](#Getting-Started)
    - [Requirements](##Requirements)
    - [General Setup](##General-Setup)
    - [Editor Setup](##Editor-Setup)
    - [Library Setup](##Library-Setup)
- [Documentation](#Documentation)
- [License](#License)

# Introduction

asdf

# Getting Started

Setting up Heart Engine is relatively simple, and it utilizes CMake and git submodules. Start by following the [General Setup](#General-Setup), and depending on if you intend to use the [Editor](#Editor-Setup) or the [Library](#Library-Setup) version of Heart, you should then follow the respective setup sections.

## Requirements

- Compiler using C++17 or higher
- CMake 3.14 or higher
- [VulkanSDK](https://vulkan.lunarg.com/) >= 1.2.198
    - Make sure to include the 64-bit debuggable shader API libraries when installing
- Currently supported platforms: Windows (x64)

## General Setup

Regardless of how you intend to use Heart Engine, you'll want to follow these initial setup steps first.

1. Clone the repo using the `--recursive` flag to ensure all submodules are downloaded
2. Make sure the VulkanSDK is accessable in your PATH via ${VULKAN_SDK} (this should happen automatically with the installer)
3. Create a `build` directory in the project root and cd into it

## Editor Setup

If you are planning on building and using the editor, you'll want to run the following commands:
```
$ cmake ../ -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=1
$ cmake --build . --config Release
```

The editor binary and associated files should build to `build/bin/Release`.

Make sure to copy the default `imgui.ini` file from the `HeartEditor` directory to the binary directory before you run the program.

## Library Setup

If you are planning on using the Heart Engine library, you'll want to run the following commands in an administrator command prompt:
```
$ cmake ../ -DCMAKE_BUILD_TYPE=Debug -DBUILD_EDITOR=0
$ cmake --build . --config Debug
$ cmake --install . --config Debug
```

# Documentation

Documenation is built using [Doxygen](https://www.doxygen.nl/).

# License

Copyright (C) 2021 [Evan Thompson](https://evanthompson.site/)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.