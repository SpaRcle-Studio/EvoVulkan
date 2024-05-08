cmake_minimum_required(VERSION 3.10.2)
project(EvoVulkanCore)

set(CMAKE_CXX_STANDARD 20)

message("[EvoVulkan] Vulkan SDK path: $ENV{VULKAN_SDK}")

find_package(Vulkan REQUIRED)

message("[EvoVulkan] Vulkan SDK Version: ${Vulkan_VERSION}")

if (EVO_VULKAN_STATIC_LIBRARY)
    add_library(EvoVulkan STATIC EvoVulkan.cxx)

    if (CYGWIN)
        #find_package(Vulkan REQUIRED FATAL_ERROR)
        # target_link_libraries(EvoVulkanCore "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    else()
        #find_package(Vulkan REQUIRED FATAL_ERROR)
        target_link_libraries(EvoVulkan PRIVATE ${Vulkan_LIBRARY})
        target_include_directories(EvoVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
        message("[EvoVulkan] Vulkan include directories (static): ${Vulkan_INCLUDE_DIRS}")
    endif()
else()
    add_library(EvoVulkan SHARED EvoVulkan.cxx)
    add_library(EvoVulkan::lib ALIAS EvoVulkan)

    #find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(EvoVulkan PUBLIC ${Vulkan_LIBRARY})
    target_include_directories(EvoVulkan PUBLIC ${Vulkan_INCLUDE_DIRS})
    message("[EvoVulkan] Vulkan include directories (public): ${Vulkan_INCLUDE_DIRS}")
endif()

target_include_directories(EvoVulkan PUBLIC inc)
