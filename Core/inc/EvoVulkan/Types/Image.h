//
// Created by Monika on 07.02.2022.
//

#ifndef EVO_VULKAN_IMAGE_H
#define EVO_VULKAN_IMAGE_H

#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Types {
    struct DLL_EVK_EXPORT ImageCreateInfo {
        ImageCreateInfo() = default;

        ImageCreateInfo(
            Memory::Allocator* pAllocator,
            EvoVulkan::Types::CmdPool* pPool,
            uint32_t _width,
            uint32_t _height,
            uint32_t _depth,
            VkImageAspectFlags _aspect,
            VkFormat _format,
            VkImageUsageFlags _usage,
            uint8_t _sampleCount,
            bool _cpuUsage,
            uint32_t _mipLevels,
            uint32_t _arrayLayers,
            VkImageCreateFlagBits _flags = VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM
        );

    public:
        Memory::Allocator* pAllocator = nullptr;
        EvoVulkan::Types::CmdPool* pPool = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
        uint32_t mipLevels = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageTiling tiling = VK_IMAGE_TILING_MAX_ENUM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
        uint8_t sampleCount = 0;
        VkImageCreateFlagBits createFlagBits = VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM;
        uint32_t arrayLayers = 1;
        bool CPUUsage = false;

        EVK_NODISCARD bool Valid() const {
            return width > 0 && height > 0 && pAllocator;
        }
    };

    class DLL_EVK_EXPORT Image : public Tools::NonCopyable {
        friend class Memory::Allocator;
    public:
        Image() = default;
        ~Image() override = default;

        Image(Image&& image) noexcept {
            m_image = std::exchange(image.m_image, {});
            m_allocation = std::exchange(image.m_allocation, {});
            m_allocator = std::exchange(image.m_allocator, {});
            m_layout = std::exchange(image.m_layout, {});
            m_info = image.m_info;
        }

        Image& operator=(Image&& image) noexcept {
            m_image = std::exchange(image.m_image, {});
            m_allocation = std::exchange(image.m_allocation, {});
            m_allocator = std::exchange(image.m_allocator, {});
            m_layout = std::exchange(image.m_layout, {});
            m_info = image.m_info;
            return *this;
        }

        bool TransitionImageLayout(VkImageLayout layout, CmdBuffer* pBuffer = nullptr) const;

    public:
        static Image Create(const ImageCreateInfo& info);

        bool Bind();

        EVK_NODISCARD VkImageLayout GetLayout() const { return m_layout; }
        EVK_NODISCARD VkFormat GetFormat() const { return m_info.format; }
        EVK_NODISCARD VkImageUsageFlags GetUsage() const { return m_info.usage; }
        EVK_NODISCARD VkImageAspectFlags GetAspect() const { return m_info.aspect; }
        EVK_NODISCARD const ImageCreateInfo& GetInfo() const { return m_info; }

        EVK_NODISCARD Image Copy() const;
        EVK_NODISCARD bool Valid() const;

        void SetLayout(VkImageLayout layout) { m_layout = layout; }

        operator VkImage() const { return m_image; } /// NOLINT

    private:
        ImageCreateInfo m_info;
        VkImage m_image = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocator m_allocator = VK_NULL_HANDLE;
        mutable VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    };
}

namespace EvoVulkan::Tools {
    EVK_MAYBE_UNUSED static VkImageView CreateImageView(const Types::Image& image, VkImageViewType viewType, uint32_t layerIndex) {
        VkImageView view = VK_NULL_HANDLE;

        VkImageViewCreateInfo viewCI = Tools::Initializers::ImageViewCreateInfo();
        viewCI.image = image;
        viewCI.viewType = viewType;
        viewCI.format = image.GetFormat();

        if (image.GetAspect() != VK_IMAGE_ASPECT_COLOR_BIT) {
            viewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        }

        if (viewCI.format == VK_FORMAT_D32_SFLOAT_S8_UINT && image.GetAspect() == VK_IMAGE_ASPECT_COLOR_BIT) {
            VK_HALT("Tools::CreateImageView() : invalid format for color image view!");
            return VK_NULL_HANDLE;
        }

        viewCI.subresourceRange.aspectMask = image.GetAspect();
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.baseArrayLayer = layerIndex;
        viewCI.subresourceRange.layerCount = image.GetInfo().arrayLayers;
        viewCI.subresourceRange.levelCount = image.GetInfo().mipLevels;

        if (vkCreateImageView(*image.GetInfo().pAllocator->GetDevice(), &viewCI, nullptr, &view) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateImageView() : failed to create image view!");
            return VK_NULL_HANDLE;
        }

        return view;
    }
}

#endif //EVO_VULKAN_IMAGE_H
