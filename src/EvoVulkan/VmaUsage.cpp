//
// Created by Monika on 15.11.2022.
//

#include <EvoVulkan/Tools/VulkanDebug.h>

#ifdef EVO_VULKAN_BUILD_VMA
    #define VMA_IMPLEMENTATION
#endif

#ifndef VMA_ASSERT
    #define VMA_ASSERT(expr) VK_ASSERT(expr)
#else
    static_assert(false, "Something went wrong!");
#endif

#include <EvoVulkan/VmaUsage.h>