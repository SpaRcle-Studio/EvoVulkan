//
// Created by Nikita on 16.05.2021.
//

#include <EvoVulkan/Types/Texture.h>
#include <EvoVulkan/Types/Image.h>
#include <EvoVulkan/Types/VmaBuffer.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Memory/Allocator.h>

uint64_t GetDataSize(uint32_t w, uint32_t h, uint8_t level) {
    uint64_t dataSize = 0;
    for (uint8_t i = 0; i < level; i++) {
        dataSize += w * h * 4 * 6;
        w /= 2;
        h /= 2;
    }
    return dataSize;
}

uint64_t GetImageSize(uint32_t w, uint32_t h, uint8_t level, uint8_t face) {
    for (uint8_t i = 0; i < level; i++) {
        w /= 2;
        h /= 2;
    }

    return (w * h * 4) * face;
}

EvoVulkan::Types::Texture::~Texture() {
    if (m_descriptorManager && (m_descriptorSet != VK_NULL_HANDLE)) {
        m_descriptorManager->FreeDescriptorSet(&m_descriptorSet);
        m_descriptorManager = nullptr;
    }

    if (!m_canBeDestroyed) {
        return;
    }

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(*m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_view != VK_NULL_HANDLE) {
        vkDestroyImageView(*m_device, m_view, nullptr);
        m_view = VK_NULL_HANDLE;
    }

    if (m_image.Valid()) {
        m_allocator->FreeImage(m_image);
    }
}

EvoVulkan::Types::Texture* EvoVulkan::Types::Texture::LoadCubeMap(
    Device *device,
    Memory::Allocator *allocator,
    CmdPool *pool,
    VkFormat format,
    int32_t width,
    int32_t height,
    const std::array<const uint8_t*, 6> &sides,
    uint32_t mipLevels,
    bool cpuUsage)
{
    if (width <= 0 || height <= 0) {
        VK_ERROR("Texture::LoadCubeMap() : incorrect texture size!");
        return nullptr;
    }

    if (mipLevels == 0) {
        mipLevels = std::floor(std::log2(EVK_MAX(width, height))) + 1;
    }

    VK_LOG("Texture::LoadCubeMap() : loading new cube map texture... \n\tWidth: " +
           std::to_string(width) + "\n\tHeight: " + std::to_string(height));

    auto&& texture = new Texture();
    {
        texture->m_width             = width;
        texture->m_height            = height;
        texture->m_mipLevels         = mipLevels;
        texture->m_format            = format;
        texture->m_descriptorManager = nullptr;
        texture->m_allocator         = allocator;
        texture->m_device            = device;
        texture->m_canBeDestroyed    = true;
        texture->m_pool              = pool;
        texture->m_filter            = VkFilter::VK_FILTER_LINEAR;
        texture->m_cubeMap           = true;
        texture->m_cpuUsage          = cpuUsage;
    }

    const VkDeviceSize imageSize = width * height * 4 * 6;

    auto&& stagingBuffer = VmaBuffer::Create(allocator, imageSize * 2); // TODO: imageSize * 2? Check correctly or fix
    if (void* data = stagingBuffer->MapData(); !data || !stagingBuffer) {
        VK_ERROR("Texture::LoadCubeMap() : failed to map memory!");
        return nullptr;
    }
    else {
        const uint64_t layerSize = imageSize / 6;
        for (uint8_t i = 0; i < 6; ++i) {
            memcpy(static_cast<uint8_t *>(data) + (layerSize * i), sides[i], layerSize);
        }
        stagingBuffer->Unmap();
    }

    auto&& imageCI = Types::ImageCreateInfo(
        texture->m_allocator,
        pool,
        texture->m_width, texture->m_height,
        1 /** depth */,
        VK_IMAGE_ASPECT_COLOR_BIT,
        texture->m_format,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT /** usage */,
        1 /** sample count */,
        false /** cpu usage */,
        texture->m_mipLevels,
        6 /** array layers */,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT /** flags */
    );

    if (!(texture->m_image = Types::Image::Create(imageCI)).Valid()) {
        VK_ERROR("Texture::LoadCubeMap() : failed to create image!");
        return nullptr;
    }

    std::vector<VkBufferImageCopy> bufferCopyRegions = { };
    for (uint8_t face = 0; face < 6; ++face) {
        for (uint8_t level = 0; level < (uint8_t) mipLevels; ++level) {
            uint64_t offset = GetDataSize(width, height, level);

            if (face != 0) {
                offset += GetImageSize(width, height, level, face);
            }

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel       = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount     = 1;
            bufferCopyRegion.imageOffset.x                   = 0;
            bufferCopyRegion.imageOffset.y                   = 0;
            bufferCopyRegion.imageOffset.z                   = 0;
            bufferCopyRegion.imageExtent.width               = width >> level;
            bufferCopyRegion.imageExtent.height              = height >> level;
            bufferCopyRegion.imageExtent.depth               = 1;
            bufferCopyRegion.bufferOffset                    = offset;
            bufferCopyRegions.emplace_back(bufferCopyRegion);
        }
    }

    auto copyCmd = Types::CmdBuffer::BeginSingleTime(device, pool);

    texture->m_image.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyCmd);

    {
        if (!copyCmd->IsBegin())
            copyCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        /// Copy the cube map faces from the staging buffer to the optimal tiled image
        vkCmdCopyBufferToImage(
                *copyCmd,
                *stagingBuffer,
                texture->m_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(bufferCopyRegions.size()),
                bufferCopyRegions.data()
        );

        copyCmd->End();
    }

    texture->m_image.TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, copyCmd);

    //!=================================================================================================================

    delete copyCmd;
    delete stagingBuffer;

    //!=================================================================================================================

    texture->m_view = Tools::CreateImageView(texture->m_image, VK_IMAGE_VIEW_TYPE_CUBE, 0);

    if (texture->m_view == VK_NULL_HANDLE) {
        VK_ERROR("Texture::LoadCubeMap() : failed to create image view!");
        return nullptr;
    }

    //!=================================================================================================================

    /// TODO: custom sampler address mode

    texture->m_sampler = Tools::CreateSampler(
        texture->m_device,
        texture->m_mipLevels,
        texture->m_filter /** min filter */,
        texture->m_filter /** mag filter */,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_COMPARE_OP_NEVER
    );

    if (texture->m_sampler == VK_NULL_HANDLE) {
        VK_ERROR("Texture::Create() : LoadCubeMap to create sampler image!");
        return nullptr;
    }

    //!=================================================================================================================

    //! make a texture descriptor
    texture->m_descriptor = {
            texture->m_sampler,
            texture->m_view,
            texture->m_image.GetLayout()
    };
    return texture;
}

EvoVulkan::Types::Texture* EvoVulkan::Types::Texture::Load(
        EvoVulkan::Types::Device *device,
        Memory::Allocator *allocator,
        Core::DescriptorManager* manager,
        EvoVulkan::Types::CmdPool *pool,
        const unsigned char *pixels,
        VkFormat format,
        int32_t width,
        int32_t height,
        uint32_t mipLevels,
        VkFilter filter,
        bool cpuUsage)
{
    if (width == 0 || height == 0) {
        VK_ERROR("Texture::Load() : invalid texture! \n\tWidth: " + std::to_string(width) + "\n\tHeight: " + std::to_string(height));
        return nullptr;
    }

    if (!pixels) {
        VK_ERROR("Texture::Load() : pixels is nullptr!");
        return nullptr;
    }

    if (!device->IsSupportLinearBlitting(format)) {
        VK_ERROR("Texture::Load() : device does not support linear blitting!");
        return nullptr;
    }

    VK_LOG("Texture::Load() : loading new texture... \n\tWidth: " +
           std::to_string(width) + "\n\tHeight: " +
           std::to_string(height) + "\n\tMip levels: " +
           std::to_string(mipLevels) + "\n\tCPU usage: " + std::string(cpuUsage ? "True" : "False"));

    auto&& pTexture = new Texture();
    {
        pTexture->m_width             = width;
        pTexture->m_height            = height;
        pTexture->m_mipLevels         = mipLevels;
        pTexture->m_format            = format;
        pTexture->m_descriptorManager = manager;
        pTexture->m_allocator         = allocator;
        pTexture->m_device            = device;
        pTexture->m_canBeDestroyed    = true;
        pTexture->m_pool              = pool;
        pTexture->m_filter            = filter;
        pTexture->m_cubeMap           = false;
        pTexture->m_cpuUsage          = cpuUsage;
    }

    auto&& stagingBuffer = VmaBuffer::Create(allocator, pTexture->m_width * pTexture->m_height * 4, (void*)pixels);
    if (!pTexture->Create(stagingBuffer)) {
        VK_ERROR("Texture::Load() : failed to create!");
        return nullptr;
    }

    return pTexture;
}

bool EvoVulkan::Types::Texture::Create(EvoVulkan::Types::VmaBuffer *stagingBuffer) {
    auto&& imageCI = Types::ImageCreateInfo(
        m_allocator, m_pool,
        m_width, m_height, 1,
        VK_IMAGE_ASPECT_COLOR_BIT,
        m_format,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT /** usage */,
        1 /** sample count */,
        m_cpuUsage /** cpu usage */,
        m_mipLevels,
        1 /** layers count */
    );

    if (!(m_image = Types::Image::Create(imageCI)).Valid()) {
        VK_ERROR("Texture::Create() : failed to create image!");
        return false;
    }

    auto&& copyCmd = Types::CmdBuffer::BeginSingleTime(m_device, m_pool);

    m_image.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyCmd);

    Tools::CopyBufferToImage(copyCmd, *stagingBuffer, m_image, m_width, m_height);

    if (m_mipLevels == 1) {
        m_image.TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, copyCmd);
    }
    else if (!GenerateMipmaps(this, copyCmd)) {
        VK_ERROR("Texture::Create() : failed to generate mip maps!");
        return false;
    }

    //!=================================================================================================================

    delete copyCmd;
    delete stagingBuffer;

    //!=================================================================================================================

    m_view = Tools::CreateImageView(m_image, m_cubeMap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D, 0);

    if (m_view == VK_NULL_HANDLE) {
        VK_ERROR("Texture::Create() : failed to create image view!");
        return false;
    }

    //!=================================================================================================================

    m_sampler = Tools::CreateSampler(
        m_device,
        m_mipLevels,
        m_filter /** min filter */,
        m_filter /** mag filter */,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        VK_COMPARE_OP_NEVER
    );

    if (m_sampler == VK_NULL_HANDLE) {
        VK_ERROR("Texture::Create() : failed to create sampler image!");
        return false;
    }

    //!=================================================================================================================

    //! make a texture descriptor
    m_descriptor = {
        m_sampler,
        m_view,
        m_image.GetLayout()
    };

    return true;
}

bool EvoVulkan::Types::Texture::GenerateMipmaps(
    EvoVulkan::Types::Texture *texture,
    EvoVulkan::Types::CmdBuffer *singleBuffer)
{
    if (!singleBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
        VK_ERROR("Texture::GenerateMipmaps() : failed to begin command buffer!");
        return false;
    }

    int32_t mipWidth  = texture->m_width;
    int32_t mipHeight = texture->m_height;

    VkImageSubresourceRange subresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel   = 0, // default
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    };

    for (uint32_t i = 1; i < texture->m_mipLevels; i++) {
        subresourceRange.baseMipLevel = i - 1;

        Tools::Insert::ImageMemoryBarrier(
            *singleBuffer,
            texture->m_image,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            subresourceRange
        );

        {
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(*singleBuffer,
                           texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);
        }

        Tools::Insert::ImageMemoryBarrier(
            *singleBuffer,
            texture->m_image,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            subresourceRange
        );

        if (mipWidth > 1)  mipWidth  /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    subresourceRange.baseMipLevel = texture->m_mipLevels - 1;
    Tools::Insert::ImageMemoryBarrier(
        *singleBuffer,
        texture->m_image,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        subresourceRange
    );

    texture->m_image.SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return singleBuffer->End();
}

EvoVulkan::Types::DescriptorSet EvoVulkan::Types::Texture::GetDescriptorSet(VkDescriptorSetLayout layout) {
    if (!m_descriptorManager) {
        VK_HALT("Texture::GetDescriptorSet() : texture have not descriptor manager!");
        return Types::DescriptorSet();
    }

    if (m_descriptorSet == VK_NULL_HANDLE) {
        static const DescriptorPool::RequestTypes type = {
                VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        };

        m_descriptorSet = m_descriptorManager->AllocateDescriptorSet(layout, type);

        auto writer = EvoVulkan::Tools::Initializers::WriteDescriptorSet(
                m_descriptorSet,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                0, this->GetDescriptorRef());

        vkUpdateDescriptorSets(*m_device, 1, &writer, 0, nullptr);
    }

    return m_descriptorSet;
}

EvoVulkan::Types::Texture::RGBAPixel EvoVulkan::Types::Texture::GetPixel(uint32_t x, uint32_t y, uint32_t z) const {
    const uint8_t channels = Tools::GetPixelChannelsCount(m_format);
    const uint8_t pixelTypeSize = Tools::GetPixelTypeSize(m_format);
    if (channels * pixelTypeSize == 0) {
        VK_ERROR("Texture::GetPixel() : unsupported format! Format: " + Tools::Convert::format_to_string(m_format));
        return {};
    }

    if (!(m_image.GetInfo().usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
        VK_ERROR("Texture::GetPixel() : image does not support VK_IMAGE_USAGE_TRANSFER_SRC_BIT!");
        return {};
    }

    if (x >= m_image.GetInfo().width || y >= m_image.GetInfo().height || z >= m_image.GetInfo().depth) {
        VK_ERROR("Texture::GetPixel() : incorrect pixel position! Width: " + std::to_string(m_image.GetInfo().width) +
                 " Height: " + std::to_string(m_image.GetInfo().height) + " Depth: " + std::to_string(m_image.GetInfo().depth));
        return {};
    }

    auto&& copyCmd = EvoVulkan::Types::CmdBuffer::BeginSingleTime(m_device, m_pool);

    auto&& pBuffer = EvoVulkan::Types::Buffer::Create(
        m_device, m_allocator,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        pixelTypeSize * channels
    );

    VkBufferImageCopy bufferCopyRegion;
    bufferCopyRegion.bufferOffset = 0;
    bufferCopyRegion.bufferRowLength = 0;
    bufferCopyRegion.bufferImageHeight = 0;
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageOffset.x = x;
    bufferCopyRegion.imageOffset.y = y;
    bufferCopyRegion.imageOffset.z = z;
    bufferCopyRegion.imageExtent.width = 1;
    bufferCopyRegion.imageExtent.height = 1;
    bufferCopyRegion.imageExtent.depth = 1;

    const auto layout = m_image.GetLayout();

    m_image.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, copyCmd);

    if (!copyCmd->IsBegin()) {
        copyCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    vkCmdCopyImageToBuffer(
        *copyCmd,
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        *pBuffer,
        1,
        &bufferCopyRegion
    );

    m_image.TransitionImageLayout(layout, copyCmd);

    copyCmd->End();
    delete copyCmd;

    RGBAPixel pixel = {};

    const auto&& GetPixelValue = [pixelTypeSize](void* pData, uint8_t offset) -> int64_t {
        switch (pixelTypeSize) {
            case 1: return ((uint8_t*)pData)[offset];
            case 2: return ((uint16_t*)pData)[offset];
            case 4: return ((uint32_t*)pData)[offset];
            case 8: return ((uint64_t*)pData)[offset];
            default:
                return 0;
        }
    };

    if (auto&& pData = pBuffer->MapData()) {
        if (channels >= 1) { pixel.r = GetPixelValue(pData, 0); }
        if (channels >= 2) { pixel.g = GetPixelValue(pData, 1); }
        if (channels >= 3) { pixel.b = GetPixelValue(pData, 2); }
        if (channels >= 4) { pixel.a = GetPixelValue(pData, 3); }
        pBuffer->Unmap();
    }

    delete pBuffer;

    return pixel;
}

