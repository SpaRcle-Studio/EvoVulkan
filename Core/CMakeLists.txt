cmake_minimum_required(VERSION 3.10.2)
project(EvoVulkanCore)

set(CMAKE_CXX_STANDARD 20)

message("[EvoVulkan] Vulkan sdk path: $ENV{VULKAN_SDK}")

find_package(Vulkan REQUIRED FATAL_ERROR)

if (EVO_VULKAN_STATIC_LIBRARY)
    add_library(EvoVulkan STATIC EvoVulkan.cxx)

    if (CYGWIN)
        #find_package(Vulkan REQUIRED FATAL_ERROR)
       # target_link_libraries(EvoVulkanCore "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    else()
        #find_package(Vulkan REQUIRED FATAL_ERROR)
        target_link_libraries(EvoVulkan ${Vulkan_LIBRARY})
        include_directories(${Vulkan_INCLUDE_DIRS})
        message("[EvoVulkan] Vulkan include directories: ${Vulkan_INCLUDE_DIRS}")
    endif()
else()
    add_library(EvoVulkan SHARED EvoVulkan.cxx)
    add_library(EvoVulkan::lib ALIAS EvoVulkan)

    #find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(EvoVulkan ${Vulkan_LIBRARY})
    include_directories(${Vulkan_INCLUDE_DIRS})
    message("[EvoVulkan] Vulkan include directories: ${Vulkan_INCLUDE_DIRS}")
endif()

target_include_directories(EvoVulkan PUBLIC inc)
