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
        struct Image {
            VkImage        m_image;
            VkImageView    m_view;
            VkDeviceMemory m_memory;
        };
    public:
        MultisampleTarget(const MultisampleTarget&) = delete;
    private:
        MultisampleTarget() = default;
        ~MultisampleTarget() = default;
    private:
        const Device*    m_device    = nullptr;
        const Swapchain* m_swapchain = nullptr;

        uint32_t m_countResolves = 0;
        std::vector<VkFormat> m_formats = {};

        Image* m_resolves = nullptr;
        Image  m_depth  = {};
    public:
        void Destroy();
        void Free() { delete this; }

        bool ReCreate(uint32_t w, uint32_t h);
    public:
        [[nodiscard]] VkImageView GetResolve(const uint32_t& id) const noexcept { return m_resolves[id].m_view; }
        [[nodiscard]] VkImage GetResolveImage(const uint32_t& id) const noexcept { return m_resolves[id].m_image; }
        [[nodiscard]] VkImageView GetDepth() const noexcept { return m_depth.m_view; }
        [[nodiscard]] uint32_t GetResolveCount() const noexcept { return m_countResolves; }
    public:
        static MultisampleTarget* Create(
                const Device* device,
                const Swapchain* swapchain,
                uint32_t w, uint32_t h,
                const std::vector<VkFormat>& formats);
    };
}

#endif //GAMEENGINE_MULTISAMPLETARGET_H
