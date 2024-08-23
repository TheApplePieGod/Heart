cmake_minimum_required(VERSION 3.22)

project(
    Heart
    VERSION 0.1
    DESCRIPTION "A 3d game engine"
)

option(HEART_BUILD_DOCS "Build documentation" OFF)
option(HEART_BUILD_EDITOR "Build the editor" OFF)
option(HEART_BUILD_RUNTIME "Build the standalone runtime" OFF)
option(HEART_BUILD_TESTS "Build the tests" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(HEART_BUILD_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        add_subdirectory("docs")
    endif()
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

if (${CMAKE_GENERATOR} STREQUAL "Xcode")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}) 
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
    set(OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}) 
endif()
set(CMAKE_CXX_STANDARD 17)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    message("Detected GNU compiler")
    set(CMAKE_CXX_FLAGS "-fpermissive")
elseif(MSVC)
    message("Detected MSVC compiler")
    add_compile_options(/MP /INCREMENTAL)
    # Override to only use level 1. Level 2 causes massive parallel performance issues
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        add_compile_definitions(_ITERATOR_DEBUG_LEVEL=1)
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Building HeartEngine in Debug mode")
    add_compile_definitions(HE_DEBUG)
else()
    message("Building HeartEngine in Release mode")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Macos intel testing
# if (APPLE)
#     set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
#     set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
# endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

# Configure & locate dotnet hostfxr
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} "scripts/configure-hostfxr.py"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE DOTNET_CONFIGURE_OUTPUT
  )
  
  list(GET DOTNET_CONFIGURE_OUTPUT 0 DOTNET_RUNTIME_DIR)
  list(GET DOTNET_CONFIGURE_OUTPUT 1 DOTNET_HOSTFXR_PATH)
  list(GET DOTNET_CONFIGURE_OUTPUT 2 DOTNET_RUNTIME_STR)
  
  message("Dotnet runtime dir: ${DOTNET_RUNTIME_DIR}")
  message("Dotnet hostfxr path: ${DOTNET_HOSTFXR_PATH}")
  message("Dotnet runtime str: ${DOTNET_RUNTIME_STR}")

  if (DOTNET_RUNTIME_DIR STREQUAL "")
    message(FATAL_ERROR "Dotnet SDK not found (ensure DOTNET_SDK is set)")
  endif()
else()
  message(FATAL_ERROR "Python must be installed to configure Heart") 
endif()

if (HEART_BUILD_RUNTIME)
    message("Building the runtime")
    add_compile_definitions(HE_DIST)
#   add_subdirectory("HeartRuntime")
endif()

add_subdirectory("Heart")
add_subdirectory("HeartScripting")
if (HEART_BUILD_EDITOR)
    message("Building the editor")
    add_subdirectory("HeartEditor")
endif()
if (HEART_BUILD_TESTS)
    message("Building the tests")
    add_subdirectory("HeartTesting")
endif()
