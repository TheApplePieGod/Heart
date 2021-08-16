cmake_minimum_required(VERSION 3.14)

project(
    Heart
    VERSION 0.1
    DESCRIPTION "A 3d game engine"
)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(HT_DEBUG)

add_subdirectory("Heart")