//
// Created by Nikita on 09.05.2021.
//

#ifndef EVOVULKAN_TEXTURE_H
#define EVOVULKAN_TEXTURE_H

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanInsert.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Types/Image.h>
#include <EvoVulkan/Types/DescriptorSet.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Complexes {
    class FrameBuffer;
}

namespace EvoVulkan::Core {
    class DescriptorManager;
}

namespace EvoVulkan::Types {
    class VmaBuffer;
    class Device;
    class CmdPool;

    class DLL_EVK_EXPORT Texture : public Tools::NonCopyable {
        friend class EvoVulkan::Complexes::FrameBuffer;
    public:
        struct RGBAPixel {
            uint64_t r, g, b, a;
        };
    private:
        Texture() = default;

    public:
        ~Texture() override;

        static bool GenerateMipmaps(Texture* texture, Types::CmdBuffer* singleBuffer);

        static Texture* LoadCubeMap(
                Device* device,
                Memory::Allocator *allocator,
                CmdPool* pool,
                VkFormat format,
                int32_t width,
                int32_t height,
                const std::array<uint8_t*, 6>& sides,
                uint32_t mipLevels = 0,
                bool cpuUsage = false);

        static Texture* Load(
                Device *device,
                Memory::Allocator *allocator,
                Core::DescriptorManager* manager,
                CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                int32_t width, int32_t height,
                uint32_t mipLevels, VkFilter,
                bool cpuUsage = false);

        static Texture* LoadAutoMip(
                Device *device,
                Memory::Allocator *allocator,
                Core::DescriptorManager* manager,
                CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                int32_t width,
                int32_t height, VkFilter filter,
                bool cpuUsage = false)
        {
            return Load(device, allocator, manager, pool, pixels, format, width, height,
                        static_cast<uint32_t>(std::floor(std::log2(EVK_MAX(width, height)))) + 1, filter, cpuUsage);
        }

        static Texture* LoadWithoutMip(
                Device *device,
                Memory::Allocator *allocator,
                Core::DescriptorManager* manager,
                CmdPool *pool,
                const unsigned char *pixels,
                VkFormat format,
                int32_t width, int32_t height, VkFilter filter,
                bool cpuUsage = false)
        {
            return Load(device, allocator, manager, pool, pixels, format, width, height, 1, filter, cpuUsage);
        }

    public:
        EVK_NODISCARD RGBAPixel GetPixel(uint32_t x, uint32_t y, uint32_t z) const;

        EVK_NODISCARD EVK_INLINE VkDescriptorImageInfo* GetDescriptorRef() noexcept { return &m_descriptor; }
        EVK_NODISCARD EVK_INLINE VkSampler GetSampler() const { return m_sampler; }
        EVK_NODISCARD EVK_INLINE VkImageLayout GetLayout() const { return m_imageLayout; }
        EVK_NODISCARD EVK_INLINE VkImageView GetImageView() const { return m_view; }
        EVK_NODISCARD EVK_INLINE VkImage GetImage() const { return m_image; }
        EVK_NODISCARD EVK_INLINE uint32_t GetWidth() const { return m_width; }
        EVK_NODISCARD EVK_INLINE uint32_t GetHeight() const { return m_height; }
        Types::DescriptorSet GetDescriptorSet(VkDescriptorSetLayout layout);

    private:
        bool Create(VmaBuffer* stagingBuffer);

    private:
        Types::Image       m_image                   = Types::Image();

        VkSampler          m_sampler                 = VK_NULL_HANDLE;
        VkImageView        m_view                    = VK_NULL_HANDLE;

        VkImageLayout      m_imageLayout             = VK_IMAGE_LAYOUT_UNDEFINED;
        VkFormat           m_format                  = VK_FORMAT_UNDEFINED;
        VkFilter           m_filter                  = VK_FILTER_MAX_ENUM;

        uint32_t           m_width                   = 0;
        uint32_t           m_height                  = 0;
        uint32_t           m_mipLevels               = 0;
        ///uint32_t           m_seed                    = 0;

        bool               m_canBeDestroyed          = false;
        bool               m_cubeMap                 = false;
        bool               m_isDestroyed             = false;
        bool               m_cpuUsage                = false;

        Types::Device*     m_device                  = nullptr;
        Types::CmdPool*    m_pool                    = nullptr;
        Memory::Allocator* m_allocator               = nullptr;
        Core::DescriptorManager* m_descriptorManager = nullptr;

        Types::DescriptorSet      m_descriptorSet     = {};
        VkDescriptorImageInfo    m_descriptor        = {};

    };
}

#endif //EVOVULKAN_TEXTURE_H
