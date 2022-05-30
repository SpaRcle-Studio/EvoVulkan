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
        friend class Device;
    protected:
        FamilyQueues() = default;
        ~FamilyQueues() override = default;

    public:
        static FamilyQueues* Find(const VkPhysicalDevice& device, const Surface* surface);

    public:
        void Destroy() override;
        void Free() override;

        void SetQueue(const VkQueue& graphics) { m_graphicsQueue = graphics; }
        void SetPresentQueue(const VkQueue& graphics) { m_presentQueue = graphics; }

        EVK_NODISCARD bool IsComplete() const override;
        EVK_NODISCARD bool IsReady() const override;
        EVK_NODISCARD uint32_t GetGraphicsIndex() const noexcept { return static_cast<uint32_t>(m_iGraphics); }
        EVK_NODISCARD uint32_t GetPresentIndex() const noexcept { return static_cast<uint32_t>(m_iPresent); }

    public:
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue  = VK_NULL_HANDLE;

    private:
        int32_t m_iGraphics = EVK_ID_INVALID;
        int32_t m_iPresent  = EVK_ID_INVALID;

    };
}

#endif //EVOVULKAN_FAMILYQUEUES_H
