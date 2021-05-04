cmake_minimum_required(VERSION 3.16)
project(EvoVulkan)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_LEGACY_CYGWIN_WIN32 0)

add_subdirectory(Core)
add_subdirectory(Depends/glfw)

find_package(Vulkan REQUIRED FATAL_ERROR)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(EvoVulkanTest main.cpp)
target_link_libraries(EvoVulkanTest EvoVulkanCore glfw)
