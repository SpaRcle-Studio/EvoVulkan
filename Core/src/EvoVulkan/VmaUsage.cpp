//
// Created by Monika on 15.11.2022.
//

#include <EvoVulkan/Tools/VulkanDebug.h>

#define VMA_IMPLEMENTATION

#ifndef VMA_ASSERT
    #define VMA_ASSERT(expr) VK_ASSERT(expr)
#else
    static_assert(false, "Something went wrong!");
#endif

#include <EvoVulkan/VmaUsage.h>