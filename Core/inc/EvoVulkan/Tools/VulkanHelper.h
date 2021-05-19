//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANHELPER_H
#define EVOVULKAN_VULKANHELPER_H

#include <vulkan/vulkan.h>

#include <iostream>

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanConverter.h>

#include <EvoVulkan/Tools/VulkanDebug.h>

namespace EvoVulkan::Tools {
    static void DestroyFences(const VkDevice& device, const std::vector<VkFence>& fences) {
        for (auto& fence : fences)
            vkDestroyFence(device, fence, nullptr);
    }

    static std::vector<VkFence> CreateFences(const VkDevice& device, uint32_t count) {
        auto waitFences = std::vector<VkFence>(count);

        // Wait fences to sync command buffer access
        VkFenceCreateInfo fenceCreateInfo = Initializers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
        for (auto& fence : waitFences) {
            auto result = vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

            if (result != VK_SUCCESS) {
                VK_ERROR("Tools::CreateFences() : failed to create vulkan fences!");
                return {};
            }
        }

        return waitFences;
    }

    static void SetImageLayout(VkCommandBuffer cmdbuffer,
                               VkImage image,
                               VkImageLayout oldImageLayout,
                               VkImageLayout newImageLayout,
                               VkImageSubresourceRange subresourceRange,
                               VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask) {
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = Initializers::ImageMemoryBarrier();
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldImageLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newImageLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0)
                {
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
                cmdbuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    static VkPresentModeKHR GetPresentMode(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, bool vsync) {
        uint32_t presentModeCount = 0;
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

        // If v-sync is not requested, try to find a mailbox mode
        // It's the lowest latency non-tearing present mode available
        if (!vsync)
            for (size_t i = 0; i < presentModeCount; i++) {
                if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }

        return swapchainPresentMode;
    }

    static VkFormat GetDepthFormat(const VkPhysicalDevice& physicalDevice) {
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;

        //! Find supported depth format
        //! We prefer 24 bits of depth and 8 bits of stencil, but that may not be supported by all implementations
        //std::vector<VkFormat> depthFormats = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };

        std::vector<VkFormat> depthFormats = {
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
        };

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
            VK_ERROR(pCallbackData->pMessage);
            //printf("VkDebugReportCallback: %s\n", pCallbackData->pMessage);
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
