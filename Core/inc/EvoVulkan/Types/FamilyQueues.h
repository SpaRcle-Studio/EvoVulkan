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

    class FamilyQueues : public IVkObject {
        friend class Device;
    private:
        FamilyQueues()  = default;
        ~FamilyQueues() = default;
    public:
        FamilyQueues(const FamilyQueues&) = delete;
    private:
        int m_iGraphics = -1;
        int m_iPresent  = -1;
    public:
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    public:
        [[nodiscard]] bool IsComplete() const override;
        [[nodiscard]] bool IsReady()    const override;

        void SetQueue(const VkQueue& graphics) {
            this->m_graphicsQueue = graphics;
        }

        [[nodiscard]] unsigned int GetGraphicsIndex() const noexcept { return (unsigned int)m_iGraphics; }
        [[nodiscard]] unsigned int GetPresentIndex()  const noexcept { return (unsigned int)m_iPresent;  }
    public:
        static FamilyQueues* Find(const VkPhysicalDevice& device, const Surface* surface);

        void Destroy() override;
        void Free()    override;
    };
}

#endif //EVOVULKAN_FAMILYQUEUES_H
