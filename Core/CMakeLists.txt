cmake_minimum_required(VERSION 3.10.2)
project(EvoVulkanCore)

set(CMAKE_CXX_STANDARD 20)

option(EVO_VULKAN_BUILD_VMA "Build Vulkan Memory Allocator" ON)

if (EVO_VULKAN_BUILD_VMA)
    add_compile_definitions(EVO_VULKAN_BUILD_VMA)
endif()

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

if (EVO_VULKAN_TRACY_PROFILER_PATH)
    message(STATUS "Tracy profiler enabled for EvoVulkan. Profiler path: ${EVO_VULKAN_TRACY_PROFILER_PATH}")
    add_definitions(-DEVO_VULKAN_TRACY_PROFILER)
    target_include_directories(EvoVulkan PRIVATE ${EVO_VULKAN_TRACY_PROFILER_PATH}/tracy)
endif()

if (UNIX)
    target_compile_options(EvoVulkan PRIVATE -fPIC)
endif()

target_include_directories(EvoVulkan PUBLIC inc)
