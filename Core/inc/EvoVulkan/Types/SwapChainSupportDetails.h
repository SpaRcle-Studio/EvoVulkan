//
// Created by Nikita on 14.04.2021.
//

#ifndef EVOVULKAN_SWAPCHAINSUPPORTDETAILS_H
#define EVOVULKAN_SWAPCHAINSUPPORTDETAILS_H

#include <EvoVulkan/macros.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/VulkanConverter.h>

namespace EvoVulkan::Types {
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        m_capabilities = {};
        std::vector<VkSurfaceFormatKHR> m_formats      = {};
        std::vector<VkPresentModeKHR>   m_presentModes = {};

        bool                            m_complete     = false;
    };

    static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) {
        SwapChainSupportDetails details = {};

        auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.m_capabilities);
        if (result != VK_SUCCESS) {
            Tools::VkDebug::Error("Types::QuerySwapChainSupport() : failed. Reason: "
                + EvoVulkan::Tools::Convert::result_to_description(result));
            return {};
        }

        uint32_t formatCount = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (result != VK_SUCCESS) {
            Tools::VkDebug::Error("Types::QuerySwapChainSupport() : failed. Reason: "
                                  + EvoVulkan::Tools::Convert::result_to_description(result));
            return {};
        }

        if (formatCount != 0) {
            details.m_formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.m_formats.data());
        }

        uint32_t presentModeCount;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (result != VK_SUCCESS) {
            Tools::VkDebug::Error("Types::QuerySwapChainSupport() : failed. Reason: "
                                  + EvoVulkan::Tools::Convert::result_to_description(result));
            return {};
        }

        if (presentModeCount != 0) {
            details.m_presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.m_presentModes.data());
        }

        details.m_complete = true;

        return details;
    }
}

#endif //EVOVULKAN_SWAPCHAINSUPPORTDETAILS_H
