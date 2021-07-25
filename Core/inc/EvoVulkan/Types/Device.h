//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_DEVICE_H
#define EVOVULKAN_DEVICE_H

#include <vulkan/vulkan.h>

#include <EvoVulkan/Types/SwapChainSupportDetails.h>
#include <EvoVulkan/Tools/VulkanHelper.h>
#include <EvoVulkan/Types/FamilyQueues.h>

#include <map>

namespace EvoVulkan::Types {
    class Device;

    struct DeviceMemory {
        friend class Device;
    private:
        uint64_t       m_size;
        VkDeviceMemory m_memory;
    public:
        DeviceMemory() {
            m_size   = 0;
            m_memory = VK_NULL_HANDLE;
        }
    public:
        [[nodiscard]] bool Ready() const { return m_memory != VK_NULL_HANDLE && m_size > 0; }
    public:
        operator VkDeviceMemory() const {
            return m_memory;
        }
    };

    class Device {
    public:
        Device(const Device&) = delete;
    private:
        ~Device() = default;
        Device()  = default;
    private:
        uint64_t                         m_deviceMemoryAllocSize   = 0;
        uint32_t                         m_allocHeapsCount         = 0;

        VkPhysicalDevice                 m_physicalDevice          = VK_NULL_HANDLE;
        VkDevice                         m_logicalDevice           = VK_NULL_HANDLE;

        bool                             m_enableSamplerAnisotropy = false;
        float                            m_maxSamplerAnisotropy    = 0.f;

        VkPhysicalDeviceMemoryProperties m_memoryProperties        = {};

        std::string                      m_deviceName              = "Unknown";

        //! don't use for VkAttachmentDescription
        //! for multisampling.rasterizationSamples and images
        VkSampleCountFlagBits            m_maxCountMSAASamples     = VK_SAMPLE_COUNT_1_BIT;

        //! for deviceFeatures and multisampling
        bool                             m_enableSampleShading     = false;
    public:
        FamilyQueues*                    m_familyQueues            = nullptr;
    public:
        static Device* Create(const VkPhysicalDevice& physicalDevice,
                              const VkDevice& logicalDevice,
                              FamilyQueues* familyQueues,
                              const bool& enableSampleShading,
                              bool multisampling,
                              int32_t sampleCount = -1);
        void Free();
    public:
        [[nodiscard]] uint64_t GetAllocatedMemorySize() const { return m_deviceMemoryAllocSize; }
        [[nodiscard]] uint64_t GetAllocatedHeapsCount() const { return m_allocHeapsCount;       }

        DeviceMemory AllocateMemory(VkMemoryAllocateInfo memoryAllocateInfo) {
            auto memory = DeviceMemory();
            memory.m_size = memoryAllocateInfo.allocationSize;
            auto result = vkAllocateMemory(this->m_logicalDevice, &memoryAllocateInfo, nullptr, &memory.m_memory);
            if (result != VK_SUCCESS || memory == VK_NULL_HANDLE) {
                VK_ERROR("Device::AllocateMemory() : failed to allocate memory! Reason: "
                         + Tools::Convert::result_to_description(result));
                return DeviceMemory();
            }
            else {
                this->m_deviceMemoryAllocSize += memory.m_size;
                this->m_allocHeapsCount++;
                return memory;
            }
        }
        bool FreeMemory(DeviceMemory* memory) {
            if (!memory) {
                Tools::VkDebug::Error("Device::FreeMemory() : memory is nullptr!");
                return false;
            }

            if (memory->m_memory != VK_NULL_HANDLE) {
                if (this->m_deviceMemoryAllocSize < memory->m_size) {
                    Tools::VkDebug::Error("Device::FreeMemory() : incorrect memory size! Something went wrong...");
                    return false;
                }

                this->m_deviceMemoryAllocSize -= memory->m_size;
                this->m_allocHeapsCount--;

                vkFreeMemory(m_logicalDevice, *memory, nullptr);
                memory->m_memory = VK_NULL_HANDLE;
                memory->m_size   = 0;
                return true;
            } else {
                Tools::VkDebug::Error("Device::FreeMemory() : vulkan memory is nullptr!");
                return false;
            }
        }

        VkCommandPool CreateCommandPool(VkCommandPoolCreateFlags flagBits) const;
    public:
        [[nodiscard]] bool IsSupportLinearBlitting(const VkFormat& imageFormat) const;

        [[nodiscard]] inline std::string GetName() const noexcept { return m_deviceName; }

        [[nodiscard]] inline bool SamplerAnisotropyEnabled() const noexcept { return m_enableSamplerAnisotropy; }
        [[nodiscard]] inline float GetMaxSamplerAnisotropy() const noexcept { return m_maxSamplerAnisotropy;    }

        [[nodiscard]] inline VkQueue GetGraphicsQueue()  const noexcept { return m_familyQueues->m_graphicsQueue;  }
        [[nodiscard]] inline bool MultisampleEnabled()  const noexcept { return m_maxCountMSAASamples != VK_SAMPLE_COUNT_1_BIT;  }
        //[[nodiscard]] inline VkQueue GetPresentQueue() const noexcept { return m_familyQueues->m_presentQueue; }

        [[nodiscard]] VkSampleCountFlagBits GetMSAASamples() const {
            return (VkSampleCountFlagBits)m_maxCountMSAASamples;
        }

        [[nodiscard]] VkPhysicalDeviceMemoryProperties GetMemoryProperties() const {
            return m_memoryProperties;
        }

        [[nodiscard]] FamilyQueues* GetQueues() const;

        [[nodiscard]] bool IsReady() const;
        bool Destroy();

        uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;
    public:
        static bool IsBetterThan(const VkPhysicalDevice& _new, const VkPhysicalDevice& _old) {
            auto _newProp = Tools::GetDeviceProperties(_new);
            auto _oldProp = Tools::GetDeviceProperties(_old);

            return _newProp.limits.maxStorageBufferRange > _oldProp.limits.maxStorageBufferRange;
        }
    public:
        operator VkDevice()         const { return m_logicalDevice;  }
        operator VkPhysicalDevice() const { return m_physicalDevice; }
    };

    //!=================================================================================================================

    static std::string GetDeviceName(const VkPhysicalDevice& physicalDevice) {
        if (!physicalDevice)
            return "Error: Device is nullptr!";

        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        return physicalDeviceProperties.deviceName;
    }

    static bool IsDeviceSuitable(
            const VkPhysicalDevice& physicalDevice,
            const VkSurfaceKHR& surface,
            const std::vector<const char*>& extensions)
    {
        if (!surface) {
            Tools::VkDebug::Error("Types::IsDeviceSuitable() : surface is nullptr!");
            return false;
        }

        if (!extensions.empty())
            if (!Tools::CheckDeviceExtensionSupport(physicalDevice, extensions)) {
                Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : device \"" +
                    Types::GetDeviceName(physicalDevice) + "\" isn't support extensions!");
                return false;
            }

        Types::SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
        if (!swapChainSupport.m_complete) {
            Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : something went wrong! Details isn't complete!");
            return false;
        }

        bool swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
        if (!swapChainAdequate) {
            Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : device \"" +
                Types::GetDeviceName(physicalDevice) + "\" isn't support swapchain!");
            return false;
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        if (!supportedFeatures.samplerAnisotropy) {
            Tools::VkDebug::Warn("Tools::IsDeviceSuitable() : device \"" +
                 Types::GetDeviceName(physicalDevice) + "\" isn't support anisotropy!");
            return false;
        }

        return true;
    }
}

#endif //EVOVULKAN_DEVICE_H
