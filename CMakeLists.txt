cmake_minimum_required(VERSION 3.16)

project(
    Heart
    VERSION 0.1
    DESCRIPTION "A 3d game engine"
)

option(HEART_BUILD_DOCS "Build documentation" OFF)
option(HEART_BUILD_EDITOR "Build the editor" ON)

if(HEART_BUILD_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        add_subdirectory("docs")
    endif()
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Building HeartEngine in Debug mode")
    add_compile_definitions(HE_DEBUG)
else()
    message("Building HeartEngine in Release mode")
endif()

add_subdirectory("Heart")
if (HEART_BUILD_EDITOR)
    add_subdirectory("HeartEditor")
endif()