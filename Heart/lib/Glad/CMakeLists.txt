cmake_minimum_required(VERSION 3.14)

project(glad)

include_directories("include")

add_library(glad STATIC "src/glad.c")