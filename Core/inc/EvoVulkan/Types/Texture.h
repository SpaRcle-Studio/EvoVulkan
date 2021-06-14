//
// Created by Nikita on 09.05.2021.
//

#ifndef EVOVULKAN_TEXTURE_H
#define EVOVULKAN_TEXTURE_H

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanInsert.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include "Device.h"

namespace EvoVulkan::Complexes {
    class FrameBuffer;
}

namespace EvoVulkan::Types {
    struct Texture {
        friend class EvoVulkan::Complexes::FrameBuffer;
    private:
        Texture() = default;
        ~Texture() = default;
    public:
        Texture(const Texture&) = delete;
    private:
        VkDeviceMemory       m_deviceMemory   = VK_NULL_HANDLE;
        VkImage              m_image          = VK_NULL_HANDLE;
        VkImageView          m_view           = VK_NULL_HANDLE;

        VkSampler            m_sampler        = VK_NULL_HANDLE;

        VkImageLayout        m_imageLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        VkFormat             m_format         = VK_FORMAT_UNDEFINED;
        uint32_t             m_width          = 0,
                             m_height         = 0;
        uint32_t             m_mipLevels      = 0;

        bool                 m_canBeDestroyed = false;

        const Types::Device* m_device         = nullptr;
    public:
        [[nodiscard]] inline VkDescriptorImageInfo GetDescriptor() const noexcept {
            return { m_sampler, m_view, m_imageLayout };
        }
    public:
        void Destroy() {
            if (!m_canBeDestroyed)
                return;

            if (m_sampler != VK_NULL_HANDLE) {
                vkDestroySampler(*m_device, m_sampler, nullptr);
                m_sampler = VK_NULL_HANDLE;
            }

            if (m_view != VK_NULL_HANDLE) {
                vkDestroyImageView(*m_device, m_view, nullptr);
                m_view = VK_NULL_HANDLE;
            }

            if (m_image != VK_NULL_HANDLE) {
                vkDestroyImage(*m_device, m_image, nullptr);
                m_image = VK_NULL_HANDLE;
            }

            if (m_deviceMemory != VK_NULL_HANDLE) {
                vkFreeMemory(*m_device, m_deviceMemory, nullptr);
                m_deviceMemory = VK_NULL_HANDLE;
            }
        }

        void Free() {
            delete this;
        }

        static bool GenerateMipmaps(Texture* texture, Types::CmdBuffer* singleBuffer);

        static Texture* Load(
                const Device *device,
                const CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                uint32_t width, uint32_t height,
                uint32_t mipLevels,
                uint8_t channels);

        static Texture* LoadAutoMip(
                const Device *device,
                const CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                uint32_t width,
                uint32_t height,
                uint8_t channels)
        {
#ifdef max
            return Load(device, pool, pixels, format, width, height,
                        static_cast<uint32_t>(std::floor(std::log2(max(width, height)))) + 1, channels);
#else
            return Load(device, pool, pixels, format, width, height,
                        static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1, channels);
#endif
        }

        static Texture* LoadWithoutMip(
                const Device *device,
                const CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                uint32_t width, uint32_t height,
                uint8_t channels)
        {
            return Load(device, pool, pixels, format, width, height, 1, channels);
        }
    };
}

#endif //EVOVULKAN_TEXTURE_H
