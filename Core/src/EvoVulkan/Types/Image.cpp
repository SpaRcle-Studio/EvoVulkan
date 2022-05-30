//
// Created by Monika on 07.02.2022.
//

#include <EvoVulkan/Types/Image.h>

bool EvoVulkan::Types::Image::Bind() {
   const auto result = vmaBindImageMemory(m_allocator, m_allocation, m_image);

#ifdef EVK_DEBUG
    if (result != VK_SUCCESS) {
        VK_ERROR("Image::Bind() : failed to bind image memory! "
                 "\n\tReason: " + Tools::Convert::result_to_string(result) +
                 "\n\tDescription: " + Tools::Convert::result_to_description(result)
        );
    }
#endif

    return result == VK_SUCCESS;
}

EvoVulkan::Types::Image EvoVulkan::Types::Image::Create(const ImageCreateInfo &info) {
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
    imageInfo.samples       = (info.mipLevels > 1 || !info.multisampling) ? VK_SAMPLE_COUNT_1_BIT : info.device->GetMSAASamples();
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (info.createFlagBits != VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM)
        imageInfo.flags = info.createFlagBits;

    Image image = info.allocator->AllocImage(imageInfo, info.CPUUsage);

    if (!image.Valid()) {
        VK_ERROR("Image::Image() : failed to create vulkan image!");
        return image;
    }

    //if (image.Bind()) {
    //    VK_ERROR("Image::Image() : failed to bind vulkan image memory!");
    //    return Image();
    //}

    return image;
}

bool EvoVulkan::Types::Image::Valid() const {
    return m_image && m_allocation && m_allocator;
}

EvoVulkan::Types::Image EvoVulkan::Types::Image::Copy() const {
    EvoVulkan::Types::Image image;

    image.m_image = m_image;
    image.m_allocation = m_allocation;
    image.m_allocator = m_allocator;

    return image;
}
