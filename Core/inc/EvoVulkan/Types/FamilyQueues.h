//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_FAMILYQUEUES_H
#define EVOVULKAN_FAMILYQUEUES_H

#include <vulkan/vulkan.h>
#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Surface;
    class Device;

    class DLL_EVK_EXPORT FamilyQueues : public IVkObject {
    public:
        FamilyQueues(VkPhysicalDevice physicalDevice, const Surface* pSurface);
        ~FamilyQueues() override;

    public:
        static FamilyQueues* Find(VkPhysicalDevice physicalDevice, const Surface* pSurface);

    public:
        EVK_NODISCARD bool Initialize(VkDevice logicalDevice);

        EVK_NODISCARD bool IsComplete() const override;
        EVK_NODISCARD bool IsReady() const override;

        EVK_NODISCARD VkQueue GetTransferQueue() const noexcept { return m_transferQueue; }
        EVK_NODISCARD VkQueue GetGraphicsQueue() const noexcept { return m_graphicsQueue; }
        EVK_NODISCARD VkQueue GetPresentQueue() const noexcept { return m_graphicsQueue; }

        EVK_NODISCARD uint32_t GetPresentIndex() const noexcept { return static_cast<uint32_t>(m_presentQueueFamilyIndex); }
        EVK_NODISCARD uint32_t GetGraphicsIndex() const noexcept { return static_cast<uint32_t>(m_graphicsQueueFamilyIndex); }
        EVK_NODISCARD uint32_t GetComputeIndex() const noexcept { return static_cast<uint32_t>(m_computeQueueFamilyIndex); }
        EVK_NODISCARD uint32_t GetTransferIndex() const noexcept { return static_cast<uint32_t>(m_transferQueueFamilyIndex); }

    private:
        bool FindIndices();

    private:
        std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;

        uint32_t m_queueFamilyPropertyCount = 0;

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_logicalDevice = VK_NULL_HANDLE;

        const Surface* m_surface = nullptr;

        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_transferQueue = VK_NULL_HANDLE;
        VkQueue m_computeQueue  = VK_NULL_HANDLE;

        int32_t m_presentQueueFamilyIndex = EVK_ID_INVALID;
        int32_t m_graphicsQueueFamilyIndex = EVK_ID_INVALID;
        int32_t m_computeQueueFamilyIndex  = EVK_ID_INVALID;
        int32_t m_transferQueueFamilyIndex = EVK_ID_INVALID;

    };
}

#endif //EVOVULKAN_FAMILYQUEUES_H
