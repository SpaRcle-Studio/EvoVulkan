//
// Created by Monika on 01.02.2022.
//

#include <EvoVulkan/Tools/DeviceTools.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
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
        VK_ERROR("Types::IsDeviceSuitable() : surface is nullptr!");
        return false;
    }

    for (auto&& extension : extensions) {
        if (!Tools::CheckDeviceExtensionSupport(physicalDevice, extension)) {
            VK_WARN("Tools::IsDeviceSuitable() : device \"" + Tools::GetDeviceName(physicalDevice) + "\" doesn't support extensions!"
                "\n\tExtension: " + std::string(extension)
            );
        }
    }

    Types::SwapChainSupportDetails swapChainSupport = Types::QuerySwapChainSupport(physicalDevice, surface);
    if (!swapChainSupport.m_complete) {
        VK_WARN("Tools::IsDeviceSuitable() : something went wrong! Details isn't complete!");
        return false;
    }

    bool swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
    if (!swapChainAdequate) {
        VK_WARN("Tools::IsDeviceSuitable() : device \"" +
                                     Tools::GetDeviceName(physicalDevice) + "\" isn't support swapchain!");
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    if (!supportedFeatures.samplerAnisotropy) {
        VK_WARN("Tools::IsDeviceSuitable() : device \"" +
                             Tools::GetDeviceName(physicalDevice) + "\" isn't support anisotropy!");
        return false;
    }

    return true;
}

bool EvoVulkan::Tools::IsBetterThan(VkPhysicalDevice const& newDevice, VkPhysicalDevice const& oldDevice) {
    const bool newIsLLVMPipe = Tools::GetDeviceName(newDevice).find("llvmpipe") != std::string::npos;
    const bool oldIsLLVMPipe = Tools::GetDeviceName(oldDevice).find("llvmpipe") != std::string::npos;

    if (newIsLLVMPipe && !oldIsLLVMPipe) {
        return false;
    }

    const bool newIsIntel = Tools::GetDeviceName(newDevice).find("Intel") != std::string::npos;
    const bool oldIsIntel = Tools::GetDeviceName(oldDevice).find("Intel") != std::string::npos;

    if (newIsIntel && !oldIsIntel) {
        return false;
    }

    auto newProp = Tools::GetDeviceProperties(newDevice);
    auto oldProp = Tools::GetDeviceProperties(oldDevice);

    auto newExtensions = Tools::GetSupportedDeviceExtensions(newDevice);
    auto oldExtensions = Tools::GetSupportedDeviceExtensions(oldDevice);

    const int64_t storageDifference = newProp.limits.maxStorageBufferRange - oldProp.limits.maxStorageBufferRange;
    const int64_t extensionsDifference = newExtensions.size() - oldExtensions.size();

    return (storageDifference + extensionsDifference) > 0;
}
