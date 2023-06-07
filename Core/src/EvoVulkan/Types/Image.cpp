//
// Created by Monika on 07.02.2022.
//

#include <EvoVulkan/Types/Image.h>

namespace EvoVulkan::Types {
    EvoVulkan::Types::ImageCreateInfo::ImageCreateInfo(
        Device *pDevice,
        Memory::Allocator *pAllocator,
        uint32_t _width,
        uint32_t _height,
        VkImageUsageFlags _usage,
        uint8_t _sampleCount
    )
        : ImageCreateInfo(
            pDevice,
            pAllocator,
            _width,
            _height,
            VK_FORMAT_UNDEFINED,
            _usage,
            _sampleCount,
            false,
            1 /** mip levels */,
            1 /** array layers */,
            VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM
        )
    { }

    ImageCreateInfo::ImageCreateInfo(
        Device* pDevice,
        Memory::Allocator* pAllocator,
        uint32_t _width,
        uint32_t _height,
        VkFormat _format,
        VkImageUsageFlags _usage,
        uint8_t _sampleCount,
        bool _cpuUsage,
        uint32_t _mipLevels,
        uint32_t _arrayLayers,
        VkImageCreateFlagBits _flags
    )
        : device(pDevice)
        , allocator(pAllocator)
        , width(_width)
        , height(_height)
        , mipLevels(_mipLevels)
        , format(_format)
        , tiling(_cpuUsage ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL)
        , usage(_usage)
        , sampleCount(_sampleCount)
        , createFlagBits(_flags)
        , arrayLayers(_arrayLayers)
        , CPUUsage(_cpuUsage)
    { }

    bool Image::Bind() {
       const auto result = vmaBindImageMemory(m_allocator, m_allocation, m_image);

        if (result == VK_SUCCESS) {
            return true;
        }

        VK_ERROR("Image::Bind() : failed to bind image memory! "
             "\n\tReason: " + Tools::Convert::result_to_string(result) +
             "\n\tDescription: " + Tools::Convert::result_to_description(result)
        );

        return false;
    }

    Image Image::Create(const ImageCreateInfo &info) {
        if (!info.Valid()) {
            VK_ERROR("Image::Create() : create info is invalid!");
            return Image();
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = info.width;
        imageInfo.extent.height = info.height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = info.mipLevels;
        imageInfo.arrayLayers   = info.arrayLayers;
        imageInfo.format        = info.format;
        imageInfo.tiling        = info.tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = info.usage;

        if (info.mipLevels > 1) {
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        }
        else if (info.sampleCount == 0) {
            imageInfo.samples = info.device->GetMSAASamples();
        }
        else {
            imageInfo.samples = Tools::Convert::IntToSampleCount(info.sampleCount);
        }

        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        if (info.createFlagBits != VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM)
            imageInfo.flags = info.createFlagBits;

        Image image = info.allocator->AllocImage(imageInfo, info.CPUUsage);

        if (!image.Valid()) {
            VK_ERROR("Image::Image() : failed to create vulkan image!");
            return image;
        }

        return image;
    }

    bool Image::Valid() const {
        return m_image && m_allocation && m_allocator;
    }

    Image Image::Copy() const {
        EvoVulkan::Types::Image image;

        image.m_image = m_image;
        image.m_allocation = m_allocation;
        image.m_allocator = m_allocator;

        return image;
    }
}