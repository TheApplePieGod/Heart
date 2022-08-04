cmake_minimum_required(VERSION 3.23)

project(
    Heart
    VERSION 0.1
    DESCRIPTION "A 3d game engine"
)

option(HEART_BUILD_DOCS "Build documentation" OFF)
option(HEART_BUILD_EDITOR "Build the editor" ON)
option(HEART_BUILD_RUNTIME "Build the standalone runtime" OFF)

if(HEART_BUILD_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        add_subdirectory("docs")
    endif()
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}) 
set(CMAKE_CXX_STANDARD 17)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    message("Detected GNU compiler")
    set(CMAKE_CXX_FLAGS "-fpermissive")
elseif(MSVC)
    message("Detected MSVC compiler")
    add_compile_options(/MP /INCREMENTAL)
endif()

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Building HeartEngine in Debug mode")
    add_compile_definitions(HE_DEBUG)
else()
    message("Building HeartEngine in Release mode")
endif()

add_subdirectory("Heart")
add_subdirectory("HeartScripting")
if (HEART_BUILD_EDITOR)
    add_subdirectory("HeartEditor")
elseif (HEART_BUILD_RUNTIME)
    add_subdirectory("HeartRuntime")
endif()