cmake_minimum_required(VERSION 3.16)
project(EvoVulkanCore)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED FATAL_ERROR)
include_directories(${Vulkan_INCLUDE_DIRS})

add_library(EvoVulkanCore STATIC
        src/VulkanKernel.cpp

        src/Device.cpp
        src/Framebuffer.cpp
        src/ImageView.cpp
        src/Pipeline.cpp
        src/FamilyQueues.cpp
        src/Surface.cpp
        src/Swapchain.cpp
        src/Synchronization.cpp
        src/CmdPool.cpp
        src/CmdBuffer.cpp

        )

target_link_libraries(EvoVulkanCore ${Vulkan_LIBRARY})

target_include_directories(EvoVulkanCore PUBLIC inc)
