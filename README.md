![Logo](https://raw.githubusercontent.com/TheApplePieGod/Heart/9fe3deb4328aec3de7c1d669e7117341dfab88f3/images/logo.png)

# Heart

Heart is a cross-platform C++ 3D game engine. It can be used to create games using the editor or as a powerful rendering API library with built-in physically-based rendering functionality.

<!--
@cond TURN_OFF_DOXYGEN
-->
# Screenshots

## Sponza Scene
![Sponza Scene](https://raw.githubusercontent.com/TheApplePieGod/Heart/61271d6393577735023f6b5f8a1021b30aafe1eb/images/screenshot1.png)

## Spheres Scene
![Spheres Scene](https://raw.githubusercontent.com/TheApplePieGod/Heart/61271d6393577735023f6b5f8a1021b30aafe1eb/images/screenshot2.png)

## Helmet Scene
![Helmet Scene](https://raw.githubusercontent.com/TheApplePieGod/Heart/61271d6393577735023f6b5f8a1021b30aafe1eb/images/screenshot3.png)

## Material Editor
![Material Editor](https://raw.githubusercontent.com/TheApplePieGod/Heart/61271d6393577735023f6b5f8a1021b30aafe1eb/images/screenshot4.png)

# Table of Contents

- [Introduction](#Introduction)
- [Getting Started](#Getting-Started)
    - [Requirements](#Requirements)
    - [General Setup](#General-Setup)
    - [Editor Setup](#Editor-Setup)
    - [Library Setup](#Library-Setup)
- [Documentation](#Documentation)
- [License](#License)
<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

As an aspiring graphics programmer with some level of experience, I decided I wanted to go all-in and create a full blown game engine. Taking some inspiration from [TheCherno](https://www.youtube.com/user/TheChernoProject) (check him out!), I have created a (very WIP) cross-platform game engine library & GUI editor that supports multiple rendering APIs.

- Platforms (x64): `Windows`
    - Coming soon: `Linux`
- Render APIs: `OpenGL`, `Vulkan`
    - Coming eventually: `Metal`

# Getting Started

Setting up Heart Engine is relatively simple, and it utilizes CMake and git submodules. Start by following the [General Setup](#General-Setup), and depending on if you intend to use the [Editor](#Editor-Setup) or the [Library](#Library-Setup) version of Heart, you should then follow the respective setup sections.

## Requirements

- Compiler using C++17
- [CMake](https://cmake.org/download/) >= 3.16
- [VulkanSDK](https://vulkan.lunarg.com/) >= 1.2.198
    - Make sure to include the 64-bit debuggable shader API libraries when installing
- [.NET Core SDK](https://dotnet.microsoft.com/en-us/download/dotnet) >= 6.0
- [Visual Studio](https://visualstudio.microsoft.com/vs/) >= 2022
    - For writing code for scripted entities
    - Using MSBuild directly will also likely work (not tested)

## General Setup

Regardless of how you intend to use Heart Engine, you'll want to follow these initial setup steps first.

1. Clone the repo using the `--recursive` flag to ensure all submodules are downloaded
2. Make sure the VulkanSDK is accessable via the `${VULKAN_SDK}` environment variable (this should happen automatically with the installer)
3. Make sure the .NET SDK is accessable via the `${DOTNET_SDK}` environment variable
4. Create a `build` directory in the project root

## Editor Setup

If you are planning on building and using the editor, you'll want to run the following commands:
```
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DHEART_BUILD_EDITOR=1
$ cmake --build . --config Release
```

The editor binary and associated files should build to `build/bin/Release`.

Make sure to copy the default `imgui.ini` file from the `HeartEditor` directory to the binary directory before you run the program.

## Library Setup

If you are planning on [installing](https://cmake.org/cmake/help/latest/command/install.html) the Heart Engine library, you'll want to run the following commands in an administrator command prompt:
```
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DHEART_BUILD_EDITOR=0
$ cmake --build . --config Debug
$ cmake --install . --config Debug
```

You should then be able to include the engine in your project like so:
```cmake
# Locate the engine package
find_package(Heart REQUIRED)

# Locate the threads package which is required by dependencies of Heart
find_package(Threads REQUIRED)

# Link all of the engine libraries
target_link_libraries(MyTarget PUBLIC Heart::Engine)

# Use the same precompiled headers that are used in the engine
# Feel free to skip this step and use your own PCH, but you'll need
# to include many of the headers in the provided PCH in order to use
# the engine's headers properly
target_precompile_headers(
    MyTarget
    PRIVATE
    "$<BUILD_INTERFACE:hepch.h>"
)

# Copy the Optick profiling library's DLL to your target's
# executable directory
add_custom_command(
    TARGET MyTarget POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Heart::OptickCore>              
    $<TARGET_FILE_DIR:MyTarget>
)
```

> ### Note for installing: in order to use higher-level engine classes like `SceneRenderer` and `EnvironmentMap` which rely on certain shaders and textures, you will need to copy the engine's `resources` folder located in the `Heart` directory to your target's executable folder. 

Otherwise, [add_subdirectory](https://cmake.org/cmake/help/latest/command/add_subdirectory.html) should be sufficient if using git submodules or another form of package management.
```cmake
add_subdirectory("path-to-Heart")
target_link_libraries(MyTarget PUBLIC Heart::Engine)
```

Here is a simple example of using the library to create an app:
```cpp
#include "Heart/Core/App.h"

int main(int argc, char** argv)
{
    Heart::Logger::Initialize();

    Heart::App* app = new Heart::App();
    app->Run();
    delete app;
    
    return 0;
}
```

# Documentation

Documenation is built using [Doxygen](https://www.doxygen.nl/). To build, run the following commands:
```
$ cd build
$ cmake .. -DHEART_BUILD_DOCS=1
$ cmake --build .
```

The docs should then be available in `build/docs/html`. Open up the `index.html` file in a browser to start browsing.

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