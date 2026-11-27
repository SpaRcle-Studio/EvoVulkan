cmake_minimum_required(VERSION 3.10.2)
project(EvoVulkan)

cmake_policy(SET CMP0074 NEW)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(EVO_VULKAN_BUILD_VMA "Build Vulkan Memory Allocator" ON)
if (EVO_VULKAN_BUILD_VMA)
    add_compile_definitions(EVO_VULKAN_BUILD_VMA)
endif()

message(STATUS "[SpaRcle Engine] [EvoVulkan] Vulkan SDK path: $ENV{VULKAN_SDK}")

find_package(Vulkan REQUIRED)

message(STATUS "[SpaRcle Engine] [EvoVulkan] Vulkan SDK Version: ${Vulkan_VERSION}")

if (EVO_VULKAN_STATIC_LIBRARY)
    add_library(EvoVulkan STATIC cxx/EvoVulkan.cxx)
    target_compile_definitions(EvoVulkan PUBLIC EVO_VULKAN_STATIC_LIBRARY)

    if (CYGWIN)

    else()
        target_link_libraries(EvoVulkan PRIVATE ${Vulkan_LIBRARY})
        target_include_directories(EvoVulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
        message(STATUS "[SpaRcle Engine] [EvoVulkan] Vulkan include directories (static): ${Vulkan_INCLUDE_DIRS}")
    endif()
else()
    add_library(EvoVulkan SHARED EvoVulkan.cxx)
    add_library(EvoVulkan::lib ALIAS EvoVulkan)
    target_compile_definitions(EvoVulkan PRIVATE EVK_EXPORTS)

    #find_package(Vulkan REQUIRED FATAL_ERROR)
    target_link_libraries(EvoVulkan PUBLIC ${Vulkan_LIBRARY})
    target_include_directories(EvoVulkan PUBLIC ${Vulkan_INCLUDE_DIRS})
    message("[EvoVulkan] Vulkan include directories (public): ${Vulkan_INCLUDE_DIRS}")
endif()

if (SR_TRACY_ENABLE AND EVO_VULKAN_TRACY_GPU_PROFILER AND EVO_VULKAN_TRACY_PROFILER_PATH)
    message(STATUS "[SpaRcle Engine] Tracy profiler enabled for EvoVulkan. Profiler path: ${EVO_VULKAN_TRACY_PROFILER_PATH}")
    target_compile_definitions(EvoVulkan PRIVATE EVO_VULKAN_TRACY_PROFILER EVO_VULKAN_TRACY_GPU_PROFILER TRACY_ENABLE TRACY_DELAYED_INIT TRACY_MANUAL_LIFETIME)
    target_include_directories(EvoVulkan PRIVATE ${EVO_VULKAN_TRACY_PROFILER_PATH}/public/tracy)
endif()

if (UNIX)
    target_compile_options(EvoVulkan PRIVATE -fPIC)
endif()

target_include_directories(EvoVulkan PUBLIC inc)
target_include_directories(EvoVulkan PUBLIC libs/VulkanMemoryAllocator/include)
