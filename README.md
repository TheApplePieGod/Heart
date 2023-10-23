![Logo](https://raw.githubusercontent.com/TheApplePieGod/Heart/9fe3deb4328aec3de7c1d669e7117341dfab88f3/images/logo.png)

# Heart

Heart is a cross-platform C++ 3D renderer and game engine which is centered around a CPU task system, an entity component system, and C# scripting for games. Powered by the [Flourish](https://github.com/TheApplePieGod/flourish) graphics library, the engine provides high speed and quality graphics. It also has experimental support for hardware-accelerated real-time ray tracing.

<!--
@cond TURN_OFF_DOXYGEN
-->

# Table of Contents

- [Screenshots](#Screenshots)
- [Introduction](#Introduction)
- [Getting Started](#Getting-Started)
    - [Requirements](#Requirements)
    - [Setup](#General-Setup)
- [Scripting](#Scripting)
    - [Setup](#Setup)
    - [Usage](#Usage)
- [Documentation](#Documentation)
- [License](#License)

# Screenshots

## Ray Tracing Enabled

![Duck Ray Tracing](https://raw.githubusercontent.com/TheApplePieGod/Heart/main/images/duck-rays.webp)
![Forest Ray Tracing](https://raw.githubusercontent.com/TheApplePieGod/Heart/main/images/rays-2.webp)
![Sponza Ray Tracing](https://raw.githubusercontent.com/TheApplePieGod/Heart/main/images/rays-3.webp)

## Standard

![Duck](https://raw.githubusercontent.com/TheApplePieGod/Heart/main/images/duck-norays.webp)
![Material Editor](https://raw.githubusercontent.com/TheApplePieGod/Heart/main/images/material-editor.webp)

<!--
@endcond TURN_OFF_DOXYGEN
-->

# Introduction

- Platforms (64 bit): `Windows`, `MacOS`
    - Coming soon: `Linux`

# Getting Started

## Requirements

- Compiler using C++17
- [CMake](https://cmake.org/download/) >= 3.23
- [.NET Core SDK](https://dotnet.microsoft.com/en-us/download/dotnet) >= 7.0
- [Visual Studio](https://visualstudio.microsoft.com/vs/) >= 2022
    - For writing code for scripted entities
    - Using MSBuild directly will also work

Ensure the requirements for [Flourish](https://github.com/TheApplePieGod/flourish#requirements) are also met.

## Setup

1. Clone the repo using the `--recursive` flag to ensure all submodules are downloaded
2. Make sure the VulkanSDK is accessable via the `${VULKAN_SDK}` environment variable
3. Make sure the .NET SDK is accessable via the `${DOTNET_SDK}` environment variable
4. Create a `build` directory in the project root

```
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release -DHEART_BUILD_EDITOR=1
$ cmake --build . --config Release
```

The editor binary and associated files should build to `build/bin/Release`.

Make sure to copy the default `imgui.ini` file from the `HeartEditor` directory to the binary directory before you run the program (unless you have a project to load).

# Scripting

The workflow for scripting within the engine is still a work in progress. To get started, use the in-engine interface to create a new project. Projects will link a NuGet package called "Heart.NET.Sdk," which contains the standard library used to interface from C# to the engine. For now, the package is not yet released on the NuGet package manager, so you'll have to build and install it manually.

## Setup

1. Ensure the [NuGet CLI](https://docs.microsoft.com/en-us/nuget/reference/nuget-exe-cli-reference) is installed
2. Navigate to the `HeartScripting` directory and open/build the `HeartScripting.sln` solution
3. Run the `CreateNugetPackage` script using either powershell or bash
4. Open your new project's visual studio solution and add the `HeartScripting` directory as a [NuGet package source](https://docs.microsoft.com/en-us/nuget/consume-packages/install-use-packages-visual-studio#package-sources)
5. Restart visual studio. The package should have been installed and should now be accessible

There are some other helpful scripts in that directory that automate some of this process.

## Usage

Entities can be given a `ScriptComponent`, which can be set to any class that inherits from the `Heart.Scene.ScriptEntity` class. All new projects come with an example entity that demonstrates this. 

Public fields in the class will be exposed to the editor where the values can be modified by hand. Only certain types are supported for this process (documentation coming soon).

After making a change in your scripts, you can either build the project directly and navigate to `File -> Reload Client Scripts` or use the shortcut `Ctrl+B` to build and reload your scripts directly in the editor.

# Documentation

Documenation is built using [Doxygen](https://www.doxygen.nl/). To build, run the following commands:
```
$ cd build
$ cmake .. -DHEART_BUILD_DOCS=1
$ cmake --build .
```

The docs should then be available in `build/docs/html`. Open up the `index.html` file in a browser to start browsing.

WARNING: It has not been updated in quite a while. It is also not particularly useful currently unless you plan on modifying the engine or using its classes directly.

# License

Copyright (C) 2023 [Evan Thompson](https://evanthompson.site/)

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
