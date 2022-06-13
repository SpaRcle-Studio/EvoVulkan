//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_DEVICETOOLS_H
#define EVOVULKAN_DEVICETOOLS_H

#include <EvoVulkan/macros.h>

namespace EvoVulkan::Tools {
    DLL_EVK_EXPORT std::string GetDeviceName(const VkPhysicalDevice& physicalDevice);

    DLL_EVK_EXPORT bool IsDeviceSuitable(
            const VkPhysicalDevice& physicalDevice,
            const VkSurfaceKHR& surface,
            const std::vector<const char*>& extensions);

    DLL_EVK_EXPORT bool IsBetterThan(const VkPhysicalDevice& _new, const VkPhysicalDevice& _old);
}

#endif //EVOVULKAN_DEVICETOOLS_H
