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
    class FamilyQueues;

    struct EvoDeviceCreateInfo {
        Instance* pInstance = nullptr;
        const Surface* pSurface = nullptr;
        std::vector<const char*> extensions;
        std::vector<const char*> validationLayers;
        bool enableSampleShading = false;
        bool rayTracing = false;
        bool multisampling = false;
        uint32_t sampleCount = 0;
    };

    class DLL_EVK_EXPORT Device : public Tools::NonCopyable {
    private:
        Device(Instance* pInstance, FamilyQueues* pQueues, VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

    public:
        ~Device() override;

        EVK_MAYBE_UNUSED static Types::Device* Create(EvoDeviceCreateInfo info);

        operator VkDevice()         const { return m_logicalDevice;  }
        operator VkPhysicalDevice() const { return m_physicalDevice; }

    public:
        EVK_NODISCARD EVK_INLINE std::string GetName() const noexcept { return m_deviceName; }
        EVK_NODISCARD EVK_INLINE bool SamplerAnisotropyEnabled() const noexcept { return m_enableSamplerAnisotropy; }
        EVK_NODISCARD EVK_INLINE float GetMaxSamplerAnisotropy() const noexcept { return m_maxSamplerAnisotropy;    }
        EVK_NODISCARD EVK_INLINE bool MultisampleEnabled() const noexcept { return m_maxCountMSAASamples != VK_SAMPLE_COUNT_1_BIT;  }
        EVK_NODISCARD EVK_INLINE Instance* GetInstance() const { return m_instance; }
        EVK_NODISCARD EVK_INLINE VkSampleCountFlagBits GetMSAASamples() const { return (VkSampleCountFlagBits)m_maxCountMSAASamples; }
        EVK_NODISCARD EVK_INLINE VkPhysicalDeviceMemoryProperties GetMemoryProperties() const { return m_memoryProperties; }

        EVK_NODISCARD VkFormat GetDepthFormat() const;
        EVK_NODISCARD uint8_t GetMSAASamplesCount() const;
        EVK_NODISCARD FamilyQueues* GetQueues() const;
        EVK_NODISCARD bool IsRayTracingSupported() const noexcept { return m_rayTracingSupported; }
        EVK_NODISCARD bool IsReady() const;
        EVK_NODISCARD bool IsExtensionSupported(const std::string& extension) const;
        EVK_NODISCARD bool IsSupportLinearBlitting(const VkFormat& imageFormat) const;
        EVK_NODISCARD bool IsSeparateDepthStencilLayoutsSupported() const { return false; }
        EVK_NODISCARD VkCommandPool CreateCommandPool(VkCommandPoolCreateFlags flagBits) const;

        uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr) const;

    private:
        bool Initialize(bool enableSampleShading, bool multisampling, uint32_t sampleCount);
        void CheckRayTracing(bool isRequested);

    private:
        FamilyQueues*                    m_familyQueues            = nullptr;

        VkPhysicalDevice                 m_physicalDevice          = VK_NULL_HANDLE;
        VkDevice                         m_logicalDevice           = VK_NULL_HANDLE;
        Types::Instance*                 m_instance                = nullptr;

        bool                             m_enableSamplerAnisotropy = false;
        float_t                          m_maxSamplerAnisotropy    = 0.f;

        VkPhysicalDeviceMemoryProperties m_memoryProperties        = { };
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RTProps  = { };

        std::string                      m_deviceName              = "Unknown";

        /// don't use for VkAttachmentDescription
        /// for multisampling.rasterizationSamples and images
        VkSampleCountFlagBits            m_maxCountMSAASamples     = VK_SAMPLE_COUNT_1_BIT;

        /// for deviceFeatures and multisampling
        bool                             m_enableSampleShading     = false;
        bool                             m_multisampling           = false;
        bool                             m_rayTracingSupported     = false;

    };
}

#endif //EVOVULKAN_DEVICE_H
