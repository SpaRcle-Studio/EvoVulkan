//
// Created by Monika on 01.02.2022.
//

#include <EvoVulkan/Tools/DeviceTools.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Types/SwapChainSupportDetails.h>

std::string EvoVulkan::Tools::GetDeviceName(VkPhysicalDevice const &physicalDevice) {
    if (!physicalDevice)
        return "Error: Device is nullptr!";

    VkPhysicalDeviceProperties physicalDeviceProperties = {};
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    return physicalDeviceProperties.deviceName;
}

bool EvoVulkan::Tools::IsDeviceSuitable(
        VkPhysicalDevice const &physicalDevice,
        VkSurfaceKHR const &surface,
        const std::vector<const char *> &extensions)
{
    if (!surface) {
        Tools::VkDebug::Error("Types::IsDeviceSuitable() : surface is nullptr!");
        return false;
    }

    if (!extensions.empty())
        if (!Tools::CheckDeviceExtensionSupport(physicalDevice, extensions)) {
            Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : device \"" +
                                         Tools::GetDeviceName(physicalDevice) + "\" isn't support extensions!");
            return false;
        }

    Types::SwapChainSupportDetails swapChainSupport = Types::QuerySwapChainSupport(physicalDevice, surface);
    if (!swapChainSupport.m_complete) {
        Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : something went wrong! Details isn't complete!");
        return false;
    }

    bool swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
    if (!swapChainAdequate) {
        Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : device \"" +
                                     Tools::GetDeviceName(physicalDevice) + "\" isn't support swapchain!");
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    if (!supportedFeatures.samplerAnisotropy) {
        Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : device \"" +
                             Tools::GetDeviceName(physicalDevice) + "\" isn't support anisotropy!");
        return false;
    }

    return true;
}

bool EvoVulkan::Tools::IsBetterThan(VkPhysicalDevice const &_new, VkPhysicalDevice const &_old)  {
    auto _newProp = Tools::GetDeviceProperties(_new);
    auto _oldProp = Tools::GetDeviceProperties(_old);

    return _newProp.limits.maxStorageBufferRange > _oldProp.limits.maxStorageBufferRange;
}
