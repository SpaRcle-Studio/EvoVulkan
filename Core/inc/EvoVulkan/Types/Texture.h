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

    struct TextureLoadInfo {
        Device* pDevice = nullptr;
        Memory::Allocator* pAllocator = nullptr;
        Core::DescriptorManager* pDescriptorManager = nullptr;
        CmdPool* pPool = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;
        int32_t width = 0;
        int32_t height = 0;
        uint32_t mipLevels = 0;
        VkFilter filter = VK_FILTER_MAX_ENUM;
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        bool cpuUsage = false;
    };

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

        static Texture* LoadCubeMap(TextureLoadInfo info, const std::array<const uint8_t*, 6>& sides);
        static Texture* Load(TextureLoadInfo info, const uint8_t* pixels);

        static Texture* LoadAutoMip(TextureLoadInfo info, const uint8_t* pixels) {
            EVK_TRACY_ZONE;
            info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(EVK_MAX(info.width, info.height)))) + 1;
            return Load(info, pixels);
        }

        static Texture* LoadWithoutMip(TextureLoadInfo info, const uint8_t* pixels) {
            EVK_TRACY_ZONE;
            info.mipLevels = 1;
            return Load(info, pixels);
        }

    public:
        EVK_NODISCARD RGBAPixel GetPixel(uint32_t x, uint32_t y, uint32_t z) const;

        EVK_NODISCARD EVK_INLINE VkDescriptorImageInfo* GetDescriptorRef() noexcept { return &m_descriptor; }
        EVK_NODISCARD EVK_INLINE VkSampler GetSampler() const { return m_sampler; }
        EVK_NODISCARD EVK_INLINE VkImageLayout GetLayout() const { return m_image.GetLayout(); }
        EVK_NODISCARD EVK_INLINE VkImageView GetImageView() const { return m_view; }
        EVK_NODISCARD EVK_INLINE const Types::Image& GetImage() const { return m_image; }
        EVK_NODISCARD EVK_INLINE uint32_t GetWidth() const { return m_loadInfo.width; }
        EVK_NODISCARD EVK_INLINE uint32_t GetHeight() const { return m_loadInfo.height; }
        Types::DescriptorSet GetDescriptorSet(VkDescriptorSetLayout layout);

    private:
        bool Create(VmaBuffer* stagingBuffer);

    private:
        Types::Image       m_image                   = Types::Image();

        VkSampler          m_sampler                 = VK_NULL_HANDLE;
        VkImageView        m_view                    = VK_NULL_HANDLE;

        bool               m_canBeDestroyed          = false;
        bool               m_cubeMap                 = false;
        TextureLoadInfo  m_loadInfo                = {};

        Types::DescriptorSet m_descriptorSet = {};
        VkDescriptorImageInfo m_descriptor = {};

    };
}

#endif //EVOVULKAN_TEXTURE_H
