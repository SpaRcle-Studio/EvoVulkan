//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_DEVICE_H
#define EVOVULKAN_DEVICE_H

#include <EvoVulkan/Memory/Allocator.h>

#include <EvoVulkan/Types/SwapChainSupportDetails.h>
#include <EvoVulkan/Tools/VulkanHelper.h>
#include <EvoVulkan/Types/FamilyQueues.h>
#include <EvoVulkan/Types/Instance.h>

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Types {
    class Device;

    struct EvoDeviceCreateInfo {
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        Types::Instance* instance;
        FamilyQueues* familyQueues;
        bool enableSampleShading;
        bool multisampling;
        int32_t sampleCount;
    };

    class Device : public Tools::NonCopyable {
    private:
        ~Device() = default;
        Device()  = default;

    public:
        operator VkDevice()         const { return m_logicalDevice;  }
        operator VkPhysicalDevice() const { return m_physicalDevice; }

    public:
        static Device* Create(const EvoDeviceCreateInfo& info);
        void Free();

        [[nodiscard]] VkCommandPool CreateCommandPool(VkCommandPoolCreateFlags flagBits) const;

    public:
        [[nodiscard]] EVK_INLINE std::string GetName() const noexcept { return m_deviceName; }
        [[nodiscard]] EVK_INLINE bool SamplerAnisotropyEnabled() const noexcept { return m_enableSamplerAnisotropy; }
        [[nodiscard]] EVK_INLINE float GetMaxSamplerAnisotropy() const noexcept { return m_maxSamplerAnisotropy;    }
        [[nodiscard]] EVK_INLINE VkQueue GetGraphicsQueue()  const noexcept { return m_familyQueues->m_graphicsQueue;  }
        [[nodiscard]] EVK_INLINE VkQueue GetPresentQueue()  const noexcept { return m_familyQueues->m_presentQueue;  }
        [[nodiscard]] EVK_INLINE bool MultisampleEnabled()  const noexcept { return m_maxCountMSAASamples != VK_SAMPLE_COUNT_1_BIT;  }
        [[nodiscard]] EVK_INLINE Instance* GetInstance() const { return m_instance; }
        [[nodiscard]] EVK_INLINE VkSampleCountFlagBits GetMSAASamples() const { return (VkSampleCountFlagBits)m_maxCountMSAASamples; }
        [[nodiscard]] EVK_INLINE VkPhysicalDeviceMemoryProperties GetMemoryProperties() const { return m_memoryProperties; }

        [[nodiscard]] FamilyQueues* GetQueues() const;
        [[nodiscard]] bool IsReady() const;
        [[nodiscard]] bool IsSupportLinearBlitting(const VkFormat& imageFormat) const;

        bool Destroy();

        uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;

    public:
        FamilyQueues*                    m_familyQueues            = nullptr;

        VkPhysicalDevice                 m_physicalDevice          = VK_NULL_HANDLE;
        VkDevice                         m_logicalDevice           = VK_NULL_HANDLE;
        Types::Instance*                 m_instance                = nullptr;

        bool                             m_enableSamplerAnisotropy = false;
        float                            m_maxSamplerAnisotropy    = 0.f;

        VkPhysicalDeviceMemoryProperties m_memoryProperties        = {};

        std::string                      m_deviceName              = "Unknown";

        //! don't use for VkAttachmentDescription
        //! for multisampling.rasterizationSamples and images
        VkSampleCountFlagBits            m_maxCountMSAASamples     = VK_SAMPLE_COUNT_1_BIT;

        //! for deviceFeatures and multisampling
        bool                             m_enableSampleShading     = false;

    };
}

#endif //EVOVULKAN_DEVICE_H
