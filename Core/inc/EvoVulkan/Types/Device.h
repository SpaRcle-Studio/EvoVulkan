//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_DEVICE_H
#define EVOVULKAN_DEVICE_H

#include <vulkan/vulkan.h>

#include <EvoVulkan/Types/SwapChainSupportDetails.h>
#include <EvoVulkan/Tools/VulkanHelper.h>
#include <EvoVulkan/Types/FamilyQueues.h>

namespace EvoVulkan::Types {
    class Device {
    public:
        Device(const Device&) = delete;
    private:
        ~Device() = default;
        Device()  = default;
    private:
        VkPhysicalDevice m_physicalDevice      = VK_NULL_HANDLE;
        VkDevice         m_logicalDevice       = VK_NULL_HANDLE;

        FamilyQueues*     m_familyQueues       = nullptr;

        //! don't use for VkAttachmentDescription
        unsigned __int8  m_maxCountMSAASamples = VK_SAMPLE_COUNT_1_BIT;

        //! for deviceFeatures and multisampling
        bool             m_enableSampleShading = false;
    public:
        static Device* Create(VkPhysicalDevice physicalDevice);
        bool Free();
    public:
        operator VkDevice()         const { return m_logicalDevice;  }
        operator VkPhysicalDevice() const { return m_physicalDevice; }
    };

    //!=================================================================================================================

    static std::string GetDeviceName(const VkPhysicalDevice& physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        return physicalDeviceProperties.deviceName;
    }

    static bool IsDeviceSuitable(
            const VkPhysicalDevice& physicalDevice,
            const VkSurfaceKHR& surface,
            const std::vector<const char*>& extensions)
    {
        bool extensionsSupported = Tools::CheckDeviceExtensionSupport(physicalDevice, extensions);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            Types::SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
            swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        return extensionsSupported && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
    }
}

#endif //EVOVULKAN_DEVICE_H
