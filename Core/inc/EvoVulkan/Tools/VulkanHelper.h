//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANHELPER_H
#define EVOVULKAN_VULKANHELPER_H

#include <vulkan/vulkan.h>

#include <iostream>

#include <EvoVulkan/Tools/VulkanDebug.h>

/*
// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
    fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(inst, "vk"#entrypoint); \
    if (fp##entrypoint == NULL)                                         \
	{																    \
	    std::cerr << "vk"#entrypoint << std::endl;                      \
        exit(1);                                                        \
    }                                                                   \
}

// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    if (fp##entrypoint == NULL)                                         \
	{																    \
	    std::cerr << "vk"#entrypoint << std::endl;                      \
        exit(1);                                                        \
    }                                                                   \
}*/

namespace EvoVulkan::Tools {
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    static VkPresentModeKHR GetPresentMode(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
        uint32_t presentModeCount;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL) != VK_SUCCESS) {
            Tools::VkDebug::Error("VulkanTools::GetPresentMode() : failed get physical device surface present modes! (count)");
            return VK_PRESENT_MODE_MAX_ENUM_KHR;
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, &presentModes[0]) != VK_SUCCESS) {
            Tools::VkDebug::Error("VulkanTools::GetPresentMode() : failed get physical device surface present modes! (data)");
            return VK_PRESENT_MODE_MAX_ENUM_KHR;
        }

        // Try to use mailbox mode
        // Low latency and non-tearing
        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (size_t i = 0; i < presentModeCount; i++) {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }

            if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        return swapchainPresentMode;
    }

    static VkFormat GetDepthFormat(const VkPhysicalDevice& physicalDevice) {
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;

        //! Find supported depth format
        //! We prefer 24 bits of depth and 8 bits of stencil, but that may not be supported by all implementations
        std::vector<VkFormat> depthFormats = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
        for (auto& format : depthFormats) {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
            //! Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures && VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                depthFormat = format;
                break;
            }
        }

        return depthFormat;
    }

    static VkSampleCountFlagBits GetMaxUsableSampleCount(const VkPhysicalDevice& physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts
                & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
    {
        if (std::string(pCallbackData->pMessage).find("Error") != std::string::npos)
            printf("VkDebugReportCallback: %s\n", pCallbackData->pMessage);
        return VK_FALSE;    // Т.к. мы не хотим чтобы вызывающая функция упала.
    }

    static VkPhysicalDeviceProperties GetDeviceProperties(const VkPhysicalDevice& physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        return physicalDeviceProperties;
    }

    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugReportCallback;
    }

    static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& extensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        //std::set<const char*> requiredExtensions(extensions.begin(), extensions.end());
        std::vector<const char*> requiredExtensions(extensions.begin(), extensions.end());

        //for (const auto& extension : availableExtensions)
        //    requiredExtensions.erase(extension.extensionName);

        for (auto & requiredExtension : requiredExtensions) {
            bool notFound = false;

            for (auto &availableExtension : availableExtensions) {
                notFound = true;
                if (strcmp(requiredExtension, availableExtension.extensionName) == 0) {
                    notFound = false;
                    break;
                }
            }

            if (notFound)
                return false;
        }

        return true;
    }

    static std::vector<VkPhysicalDevice> GetAllDevices(const VkInstance& instance) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            VkDebug::Error("VulkanTools::GetAllDevice() : failed to find GPUs with Vulkan support!");
            return {};
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        return devices;
    }
}

#endif //EVOVULKAN_VULKANHELPER_H
