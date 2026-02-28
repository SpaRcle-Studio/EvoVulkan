//
// Created by Nikita on 11.06.2021.
//

#ifndef SR_ENGINE_MULTISAMPLETARGET_H
#define SR_ENGINE_MULTISAMPLETARGET_H

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
                Types::CmdPool* cmdPool,
                Swapchain* swapchain,
                uint32_t w, uint32_t h,
                const std::vector<VkFormat>& formats,
                uint8_t sampleCount,
                uint32_t arrayLayers,
                VkImageAspectFlags depthAspect,
                VkFormat depthFormat);

    public:
        void Destroy();
        void Free() { delete this; }

        bool ReCreate(uint32_t w, uint32_t h);
        void SetSampleCount(uint8_t sampleCount);

        EVK_NODISCARD VkImageView GetResolve(const uint32_t& id) const noexcept;
        EVK_NODISCARD Types::Image& GetDepthImage() noexcept { return m_depth.m_image; }
        EVK_NODISCARD VkImageView GetDepth() const noexcept { return m_depth.m_view; }
        EVK_NODISCARD uint32_t GetResolveCount() const noexcept { return m_countResolves; }

    private:
        Device*            m_device    = nullptr;
        Memory::Allocator* m_allocator = nullptr;
        Swapchain*         m_swapchain = nullptr;
        Types::CmdPool*    m_cmdPool   = nullptr;

        VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

        uint32_t m_width = 0;
        uint32_t m_height = 0;

        uint32_t m_layersCount = 0;
        uint8_t m_sampleCount = 0;
        VkImageAspectFlags m_depthAspect = 0;

        uint32_t m_countResolves = 0;
        std::vector<VkFormat> m_formats = {};

        Image* m_resolves = nullptr;
        Image m_depth;

    };
}

#endif //SR_ENGINE_MULTISAMPLETARGET_H
