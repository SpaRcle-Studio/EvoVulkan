//
// Created by Nikita on 11.06.2021.
//

#ifndef GAMEENGINE_MULTISAMPLETARGET_H
#define GAMEENGINE_MULTISAMPLETARGET_H

#include <vulkan/vulkan.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Swapchain.h>

namespace EvoVulkan::Types {
    class MultisampleTarget {
    public:
        MultisampleTarget(const MultisampleTarget&) = delete;
    private:
        MultisampleTarget() = default;
        ~MultisampleTarget() = default;
    private:
        Device*    m_device    = nullptr;
        Swapchain* m_swapchain = nullptr;

        struct {
            VkImage        m_image;
            VkImageView    m_view;
            VkDeviceMemory m_memory;
        } m_color = {},
          m_depth = {};
    public:
        void Destroy();
        void Free() { delete this; }

        bool ReCreate(uint32_t w, uint32_t h);
    public:
        [[nodiscard]] VkImageView GetColor() const noexcept { return m_color.m_view; }
        [[nodiscard]] VkImageView GetDepth() const noexcept { return m_depth.m_view; }
    public:
        static MultisampleTarget* Create(Device* device, Swapchain* swapchain, uint32_t w, uint32_t h);
    };
}

#endif //GAMEENGINE_MULTISAMPLETARGET_H
