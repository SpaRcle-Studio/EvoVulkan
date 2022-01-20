cmake_minimum_required(VERSION 3.16)
project(EvoVulkanCore)

set(CMAKE_CXX_STANDARD 20)

add_library(EvoVulkanCore STATIC EvoVulkan.cxx
        #[[src/EvoVulkan/VulkanKernel.cpp
        src/EvoVulkan/DescriptorManager.cpp

        src/EvoVulkan/Types/MultisampleTarget.cpp
        src/EvoVulkan/Types/Device.cpp
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
        src/EvoVulkan/Types/Texture.cpp

        src/EvoVulkan/Tools/VulkanTools.cpp
        src/EvoVulkan/Tools/VulkanDebug.cpp

        src/EvoVulkan/Complexes/Framebuffer.cpp
        src/EvoVulkan/Complexes/Shader.cpp
        src/EvoVulkan/Complexes/Mesh.cpp
        inc/EvoVulkan/Tools/FileSystem.h]]
        )

if (CYGWIN)
    #find_package(Vulkan REQUIRED FATAL_ERROR)
   # target_link_libraries(EvoVulkanCore "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
else()
    find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(EvoVulkanCore ${Vulkan_LIBRARY})
    include_directories(${Vulkan_INCLUDE_DIRS})
endif()

target_include_directories(EvoVulkanCore PUBLIC inc)
