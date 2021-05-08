cmake_minimum_required(VERSION 3.16)
project(EvoVulkanCore)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED FATAL_ERROR)
include_directories(${Vulkan_INCLUDE_DIRS})

add_library(EvoVulkanCore STATIC
        src/EvoVulkan/VulkanKernel.cpp
        src/EvoVulkan/DescriptorManager.cpp

        src/EvoVulkan/Types/Device.cpp
        src/EvoVulkan/Types/Framebuffer.cpp
        src/EvoVulkan/Types/ImageView.cpp
        src/EvoVulkan/Types/Pipeline.cpp
        src/EvoVulkan/Types/FamilyQueues.cpp
        src/EvoVulkan/Types/Surface.cpp
        src/EvoVulkan/Types/Swapchain.cpp
        src/EvoVulkan/Types/Synchronization.cpp
        src/EvoVulkan/Types/CmdPool.cpp
        src/EvoVulkan/Types/CmdBuffer.cpp
        src/EvoVulkan/Types/VulkanBuffer.cpp
        src/EvoVulkan/Types/DepthStencil.cpp

        src/EvoVulkan/Tools/VulkanTools.cpp

        src/EvoVulkan/Complexes/Shader.cpp
        src/EvoVulkan/Complexes/Mesh.cpp
        )

target_link_libraries(EvoVulkanCore ${Vulkan_LIBRARY})

target_include_directories(EvoVulkanCore PUBLIC inc)
