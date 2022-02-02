//
// Created by Nikita on 09.05.2021.
//

#ifndef EVOVULKAN_TEXTURE_H
#define EVOVULKAN_TEXTURE_H

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanInsert.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/DescriptorManager.h>
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
        DeviceMemory    m_deviceMemory   = {};
        VkImage         m_image          = VK_NULL_HANDLE;
        VkImageView     m_view           = VK_NULL_HANDLE;

        VkFilter        m_filter         = VkFilter::VK_FILTER_MAX_ENUM;
        VkSampler       m_sampler        = VK_NULL_HANDLE;

        VkImageLayout   m_imageLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        VkFormat        m_format         = VK_FORMAT_UNDEFINED;
        uint32_t        m_width          = 0,
                        m_height         = 0;
        uint32_t        m_mipLevels      = 0;

        uint32_t        m_seed           = 0;

        bool            m_canBeDestroyed = false;
        bool            m_cubeMap        = false;
        bool            m_isDestroyed    = false;

        Types::Device*  m_device         = nullptr;
        Types::CmdPool* m_pool           = nullptr;

        Core::DescriptorSet      m_descriptorSet     = {};
        Core::DescriptorManager* m_descriptorManager = nullptr;
        VkDescriptorImageInfo    m_descriptor        = {};
    public:
        [[nodiscard]] inline VkDescriptorImageInfo* GetDescriptorRef() noexcept { return &m_descriptor; }
    public:
        void RandomizeSeed() { m_seed = rand() % 10000; }
    public:
        [[nodiscard]] inline VkSampler GetSampler() const { return m_sampler; }
        [[nodiscard]] inline VkImageLayout GetLayout() const { return m_imageLayout; }
        [[nodiscard]] inline VkImageView GetImageView() const { return m_view; }
        [[nodiscard]] inline VkImage GetImage() const { return m_image; }
        [[nodiscard]] inline uint32_t GetWidth() const { return m_width; }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_height; }
        [[nodiscard]] inline uint32_t GetSeed() const { return m_seed; }
    private:
        bool Create(Buffer* stagingBuffer);
    public:
        Core::DescriptorSet GetDescriptorSet(VkDescriptorSetLayout layout);

        void Destroy();

        void Free() {
            if (!m_isDestroyed)
                VK_ERROR("Texture::Free() : texture isn't destroyed!");

            delete this;
        }

        static bool GenerateMipmaps(Texture* texture, Types::CmdBuffer* singleBuffer);

        static Texture* LoadCubeMap(
                Device* device,
                CmdPool* pool,
                VkFormat format,
                uint32_t width,
                uint32_t height,
                const std::array<uint8_t*, 6>& sides,
                uint32_t mipLevels = 0);

        static Texture* Load(
                Device *device,
                Core::DescriptorManager* manager,
                CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                uint32_t width, uint32_t height,
                uint32_t mipLevels, VkFilter filter);

        static Texture* LoadAutoMip(
                Device *device,
                Core::DescriptorManager* manager,
                CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                uint32_t width,
                uint32_t height, VkFilter filter)
        {
#ifdef max
            return Load(device, manager, pool, pixels, format, width, height,
                        static_cast<uint32_t>(std::floor(std::log2(max(width, height)))) + 1, filter);
#else
            return Load(device, manager, pool, pixels, format, width, height,
                        static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1, filter);
#endif
        }

        static Texture* LoadWithoutMip(
                Device *device,
                Core::DescriptorManager* manager,
                CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                uint32_t width, uint32_t height, VkFilter filter)
        {
            return Load(device, manager, pool, pixels, format, width, height, 1, filter);
        }
    };
}

#endif //EVOVULKAN_TEXTURE_H
