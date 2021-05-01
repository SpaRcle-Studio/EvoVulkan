//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_FAMILYQUEUES_H
#define EVOVULKAN_FAMILYQUEUES_H

#include <vulkan/vulkan.h>

namespace EvoVulkan::Types {
    class Surface;

    class FamilyQueues {
    private:
        FamilyQueues()  = default;
        ~FamilyQueues() = default;
    public:
        FamilyQueues(const FamilyQueues&) = delete;
    private:
        int m_iGraphics = -1;
        int m_iPresent  = -1;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue  = VK_NULL_HANDLE;
    public:
        [[nodiscard]] bool IsComplete() const;
        [[nodiscard]] bool IsReady()    const;

        void SetQueues(const VkQueue& graphics, const VkQueue& present) {
            this->m_graphicsQueue = graphics;
            this->m_presentQueue  = present;
        }

        [[nodiscard]] unsigned int GetGraphicsIndex() const noexcept { return (unsigned int)m_iGraphics; }
        [[nodiscard]] unsigned int GetPresentIndex() const noexcept  { return (unsigned int)m_iPresent;  }
    public:
        static FamilyQueues* Find(const VkPhysicalDevice& device, const Surface* surface);

        void Destroy();
    };
}

#endif //EVOVULKAN_FAMILYQUEUES_H
