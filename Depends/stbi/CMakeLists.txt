cmake_minimum_required(VERSION 3.10.2)
project(stbi)

set(CMAKE_CXX_STANDARD 20)

add_library(stbi STATIC src/stbi.c)

target_include_directories(stbi PUBLIC inc)
