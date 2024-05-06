cmake_minimum_required(VERSION 3.10.2)
project(EvoVulkan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(EVO_VULKAN_USE_OWN_GLFW "" ON)

#set(CMAKE_LEGACY_CYGWIN_WIN32 0)

add_subdirectory(Core)

if (EVO_VULKAN_USE_OWN_GLFW)
    add_subdirectory(Depends/glfw)
endif()

add_subdirectory(Depends/stbi)
add_subdirectory(Depends/cmp_core)

add_executable(EvoVulkanTest main.cpp)

if (EVO_VULKAN_STATIC_LIBRARY)
    if (EVO_VULKAN_USE_OWN_GLFW)
        add_subdirectory(EvoVulkanTest glfw)
        ### TODO: build own glfw if it is not found
    endif()

    target_link_libraries(EvoVulkanTest EvoVulkanCore stbi CMP_Core)

    target_include_directories(EvoVulkanTest PUBLIC Depends/inc)
    target_include_directories(EvoVulkan PUBLIC Depends/cmp_core/source)
else()
    target_link_libraries(EvoVulkanTest EvoVulkan::lib glfw stbi CMP_Core)

    add_custom_command(TARGET EvoVulkanTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/Core/EvoVulkan.dll"
            ${CMAKE_BINARY_DIR})

    target_include_directories(EvoVulkanTest PUBLIC Depends/inc)
    target_include_directories(EvoVulkan PUBLIC Depends/cmp_core/source)
endif()