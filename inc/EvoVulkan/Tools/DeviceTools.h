//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_DEVICETOOLS_H
#define EVOVULKAN_DEVICETOOLS_H

#include <EvoVulkan/macros.h>

namespace EvoVulkan::Tools {
    DLL_EVK_EXPORT std::string ToLower(std::string str) noexcept;

    struct DeviceSelectionInfo {
        VkPhysicalDeviceType type = VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM;
        std::string name;
        bool llvmPipe = false;
        uint32_t extensions = 0;
        uint64_t heapsSize = 0;
        VkPhysicalDevice device = VK_NULL_HANDLE;
    };

    DLL_EVK_EXPORT std::string GetDeviceName(const VkPhysicalDevice& physicalDevice);

    DLL_EVK_EXPORT bool IsDeviceSuitable(
            const VkPhysicalDevice& physicalDevice,
            const VkSurfaceKHR& surface,
            const std::vector<const char*>& extensions);

    DLL_EVK_EXPORT VkPhysicalDevice SelectBetterDevice(const std::vector<DeviceSelectionInfo>& devices);
    DLL_EVK_EXPORT VkPhysicalDevice SelectBetterDeviceByProperties(const std::vector<DeviceSelectionInfo>& devices);
    DLL_EVK_EXPORT DeviceSelectionInfo GetSelectionDeviceInfo(const VkPhysicalDevice& device);
    DLL_EVK_EXPORT bool IsBetterThan(const VkPhysicalDevice& _new, const VkPhysicalDevice& _old);
}

#endif //EVOVULKAN_DEVICETOOLS_H
