//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANHELPER_H
#define EVOVULKAN_VULKANHELPER_H

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanConverter.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

#define EVSafeFreeObject(object) \
    if (object) {                \
        delete object;           \
        object = nullptr;        \
    }                            \

namespace EvoVulkan::Tools {
    EVK_MAYBE_UNUSED static bool IsFormatInRange(VkFormat format) {
        return format >= VK_FORMAT_UNDEFINED && format <= VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT;
    }

    EVK_MAYBE_UNUSED static void DestroyFences(const VkDevice& device, const std::vector<VkFence>& fences) {
        for (auto& fence : fences)
            vkDestroyFence(device, fence, nullptr);
    }

    EVK_MAYBE_UNUSED static std::vector<VkFence> CreateFences(const VkDevice& device, uint32_t count) {
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

    EVK_MAYBE_UNUSED static void SetImageLayout(VkCommandBuffer cmdbuffer,
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

    EVK_MAYBE_UNUSED static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    EVK_MAYBE_UNUSED static VkPresentModeKHR GetPresentMode(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, bool vsync) {
        uint32_t presentModeCount = 0;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL) != VK_SUCCESS) {
            VK_ERROR("VulkanTools::GetPresentMode() : failed get physical device surface present modes! (count)");
            return VK_PRESENT_MODE_MAX_ENUM_KHR;
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, &presentModes[0]) != VK_SUCCESS) {
            VK_ERROR("VulkanTools::GetPresentMode() : failed get physical device surface present modes! (data)");
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

    EVK_MAYBE_UNUSED static VkSampleCountFlagBits GetMaxUsableSampleCount(const VkPhysicalDevice& physicalDevice) {
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

    EVK_MAYBE_UNUSED static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
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

    EVK_MAYBE_UNUSED static VkPhysicalDeviceProperties GetDeviceProperties(const VkPhysicalDevice& physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        return physicalDeviceProperties;
    }

    EVK_MAYBE_UNUSED static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugReportCallback;
    }

    EVK_MAYBE_UNUSED static bool IsExtensionSupported(const VkPhysicalDevice& device, const std::string& requiredExtension) {
        if (!device) {
            VK_ERROR("IsExtensionSupported() : device is nullptr!");
            return false;
        }

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::vector<std::string> extensions;
        extensions.reserve(extensionCount);

        for (auto&& extension : availableExtensions) {
            if (requiredExtension == extension.extensionName) {
                return true;
            }
        }

        return false;
    }

    EVK_MAYBE_UNUSED static std::vector<std::string> GetSupportedDeviceExtensions(const VkPhysicalDevice& device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::vector<std::string> extensions;
        extensions.reserve(extensionCount);

        for (auto&& extension : availableExtensions) {
            extensions.emplace_back(extension.extensionName);
        }

        return extensions;
    }

    EVK_MAYBE_UNUSED static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& extensions) {
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

    EVK_MAYBE_UNUSED static std::vector<VkPhysicalDevice> GetAllDevices(const VkInstance& instance) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            VK_ERROR("VulkanTools::GetAllDevice() : failed to find GPUs with Vulkan support!");
            return {};
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        return devices;
    }
}

#endif //EVOVULKAN_VULKANHELPER_H
