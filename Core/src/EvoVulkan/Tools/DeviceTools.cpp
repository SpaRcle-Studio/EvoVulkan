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
        const VkPhysicalDevice& physicalDevice,
        const VkSurfaceKHR& surface,
        const std::vector<const char*>& extensions)
{
    for (auto&& extension : extensions) {
        if (!Tools::CheckDeviceExtensionSupport(physicalDevice, extension)) {
            VK_WARN("Tools::IsDeviceSuitable() : device \"" + Tools::GetDeviceName(physicalDevice) + "\" doesn't support extensions!"
                "\n\tExtension: " + std::string(extension)
            );
        }
    }

    if (surface) {
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

    auto newProp = Tools::GetDeviceProperties(newDevice);
    auto oldProp = Tools::GetDeviceProperties(oldDevice);

    auto newExtensions = Tools::GetSupportedDeviceExtensions(newDevice);
    auto oldExtensions = Tools::GetSupportedDeviceExtensions(oldDevice);

    const int64_t storageDifference = newProp.limits.maxStorageBufferRange - oldProp.limits.maxStorageBufferRange;
    const int64_t extensionsDifference = newExtensions.size() - oldExtensions.size();

    return (storageDifference + extensionsDifference) > 0;
}

EvoVulkan::Tools::DeviceSelectionInfo EvoVulkan::Tools::GetSelectionDeviceInfo(const VkPhysicalDevice& device) {
    auto&& deviceName = Tools::ToLower(Tools::GetDeviceName(device));

    DeviceSelectionInfo info = {};

    info.name = Tools::GetDeviceName(device);
    info.device = device;
    info.llvmPipe = deviceName.find("llvmpipe") != std::string::npos;
    info.extensions = Tools::GetSupportedDeviceExtensions(device).size();

    auto&& deviceProperties = Tools::GetDeviceProperties(device);
    info.type = deviceProperties.deviceType;

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

    auto&& heapsPointer = memoryProperties.memoryHeaps;
    auto&& heaps = std::vector<VkMemoryHeap>(heapsPointer, heapsPointer + memoryProperties.memoryHeapCount);

    for (const auto& heap : heaps) {
        if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            info.heapsSize += heap.size;
        }
    }

    return info;
}

std::string EvoVulkan::Tools::ToLower(std::string str) noexcept {
    for (char& t : str)
        t = tolower(t);
    return str;
}

VkPhysicalDevice EvoVulkan::Tools::SelectBetterDevice(const std::vector<DeviceSelectionInfo>& devices) {
    if (devices.empty()) {
        return VK_NULL_HANDLE;
    }

    if (devices.size() == 1) {
        return devices.front().device;
    }

    std::string log = "Tools::SelectBetterDevice() : selecting better device:";
    for (auto&& device : devices) {
        log += "\n\tname = \"" + device.name +
                "\", type = " + Tools::Convert::physical_device_type_to_string(device.type) +
                ", llvmpipe = " + (device.llvmPipe ? "true" : "false") +
                ", extensions = " + std::to_string(device.extensions) +
                ", heapsSize = " + std::to_string(device.heapsSize)
        ;
    }

    VK_LOG(log);

    std::vector<DeviceSelectionInfo> llvmPipe;
    std::vector<DeviceSelectionInfo> integrated;
    std::vector<DeviceSelectionInfo> discrete;
    std::vector<DeviceSelectionInfo> virtualGPU;
    std::vector<DeviceSelectionInfo> cpu;
    std::vector<DeviceSelectionInfo> other;

    for (auto&& device : devices) {
        if (device.llvmPipe) {
            llvmPipe.emplace_back(device);
            continue;
        }

        switch (device.type) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER: other.emplace_back(device); break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: integrated.emplace_back(device); break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: discrete.emplace_back(device); break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: virtualGPU.emplace_back(device); break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: cpu.emplace_back(device); break;
            case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
            default:
                VK_HALT("Unresolved situation!");
                break;
        }
    }

    if (!discrete.empty()) {
        return SelectBetterDeviceByProperties(discrete);
    }

    if (!integrated.empty()) {
        return SelectBetterDeviceByProperties(discrete);
    }

    if (!virtualGPU.empty()) {
        return SelectBetterDeviceByProperties(discrete);
    }

    if (!cpu.empty()) {
        return SelectBetterDeviceByProperties(discrete);
    }

    if (!llvmPipe.empty()) {
        return SelectBetterDeviceByProperties(discrete);
    }

    if (!other.empty()) {
        return SelectBetterDeviceByProperties(discrete);
    }

    VK_HALT("Unresolved situation!");

    return VK_NULL_HANDLE;
}

int32_t SelectBetterDeviceByPropertiesComparator(const EvoVulkan::Tools::DeviceSelectionInfo& a, const EvoVulkan::Tools::DeviceSelectionInfo& b)
{
    if (a.extensions < b.extensions) {
        return -1;
    }

    if (a.extensions == b.extensions) {
        return 0;
    }

    ///////////////////////////////////////////

    if (a.heapsSize < b.heapsSize) {
        return -1;
    }

    if (a.heapsSize == b.heapsSize) {
        return 0;
    }

    return 1;
}

VkPhysicalDevice EvoVulkan::Tools::SelectBetterDeviceByProperties(const std::vector<DeviceSelectionInfo>& devices) {
    auto sorted = devices;

    std::stable_sort(sorted.begin(), sorted.end(), [](const EvoVulkan::Tools::DeviceSelectionInfo& a, const EvoVulkan::Tools::DeviceSelectionInfo& b) -> bool {
        return a.extensions > b.extensions;
    });

    std::stable_sort(sorted.begin(), sorted.end(), [](const EvoVulkan::Tools::DeviceSelectionInfo& a, const EvoVulkan::Tools::DeviceSelectionInfo& b) -> bool {
        return a.extensions == b.extensions && a.heapsSize > b.heapsSize;
    });

    auto&& device = sorted.front();
    VK_LOG("Tools::SelectBetterDeviceByProperties() : selected device is \"" + device.name + "\"");
    return device.device;
}
