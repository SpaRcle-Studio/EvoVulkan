cmake_minimum_required(VERSION 3.16)
project(EvoVulkan)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_LEGACY_CYGWIN_WIN32 0)

add_subdirectory(Core)
add_subdirectory(Depends/glfw)
add_subdirectory(Depends/stbi)
add_subdirectory(Depends/cmp_core)

find_package(Vulkan REQUIRED FATAL_ERROR)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(EvoVulkanTest main.cpp)
target_link_libraries(EvoVulkanTest EvoVulkanCore glfw stbi CMP_Core)

target_include_directories(EvoVulkanTest PUBLIC Depends/inc)
target_include_directories(EvoVulkanCore PUBLIC Depends/cmp_core/source)
