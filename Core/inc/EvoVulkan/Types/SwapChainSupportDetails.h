//
// Created by Nikita on 14.04.2021.
//

#ifndef EVOVULKAN_SWAPCHAINSUPPORTDETAILS_H
#define EVOVULKAN_SWAPCHAINSUPPORTDETAILS_H

#include <vector>
#include <vulkan/vulkan.h>

namespace EvoVulkan::Types {
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        m_capabilities = {};
        std::vector<VkSurfaceFormatKHR> m_formats      = {};
        std::vector<VkPresentModeKHR>   m_presentModes = {};
    };

    static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.m_capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.m_formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.m_formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.m_presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.m_presentModes.data());
        }

        return details;
    }
}

#endif //EVOVULKAN_SWAPCHAINSUPPORTDETAILS_H
