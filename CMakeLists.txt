cmake_minimum_required(VERSION 3.14)

project(
    Heart
    VERSION 0.1
    DESCRIPTION "A 3d game engine"
)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Building HeartEngine in Debug mode")
    add_compile_definitions(HE_DEBUG)
else()
    message("Building HeartEngine in Release mode")
endif()


add_subdirectory("HeartEditor")
add_subdirectory("Heart")