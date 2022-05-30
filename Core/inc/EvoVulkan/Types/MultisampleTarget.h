//
// Created by Nikita on 11.06.2021.
//

#ifndef GAMEENGINE_MULTISAMPLETARGET_H
#define GAMEENGINE_MULTISAMPLETARGET_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/Image.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Types {
    class DLL_EVK_EXPORT MultisampleTarget : public Tools::NonCopyable {
        struct Image {
            Types::Image m_image;
            VkImageView m_view = VK_NULL_HANDLE;
        };
    private:
        MultisampleTarget() = default;
        ~MultisampleTarget() override = default;

    public:
        static MultisampleTarget* Create(
                Device* device,
                Memory::Allocator* allocator,
                Swapchain* swapchain,
                uint32_t w, uint32_t h,
                const std::vector<VkFormat>& formats,
                bool multisampling);

    public:
        void Destroy();
        void Free() { delete this; }

        bool ReCreate(uint32_t w, uint32_t h);

        EVK_NODISCARD VkImageView GetResolve(const uint32_t& id) const noexcept { return m_resolves[id].m_view; }
        EVK_NODISCARD VkImage GetDepthImage() const noexcept { return m_depth.m_image; }
        EVK_NODISCARD VkImageView GetDepth() const noexcept { return m_depth.m_view; }
        EVK_NODISCARD uint32_t GetResolveCount() const noexcept { return m_countResolves; }

    private:
        Device*            m_device    = nullptr;
        Memory::Allocator* m_allocator = nullptr;
        Swapchain*         m_swapchain = nullptr;

        bool m_multisampling = true;

        uint32_t m_countResolves = 0;
        std::vector<VkFormat> m_formats = {};

        Image* m_resolves = nullptr;
        Image m_depth;

    };
}

#endif //GAMEENGINE_MULTISAMPLETARGET_H
