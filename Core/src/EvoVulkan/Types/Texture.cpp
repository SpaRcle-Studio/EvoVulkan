//
// Created by Nikita on 16.05.2021.
//

#include <EvoVulkan/Types/Texture.h>

EvoVulkan::Types::Texture* EvoVulkan::Types::Texture::Load(
        EvoVulkan::Types::Device *device,
        EvoVulkan::Types::CmdPool *pool,
        const unsigned char *pixels,
        VkFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkFilter filter)
{
    if (!pixels) {
        VK_ERROR("Texture::Load() : pixels is nullptr!");
        return nullptr;
    }

    bool needBlit = true;
    if (!device->IsSupportLinearBlitting(format)) {
        VK_ERROR("Texture::Load() : device does not support linear blitting!");
        needBlit = false;
        return nullptr;
    }

    VK_LOG("Texture::Load() : loading new texture... \n\tWidth: " +
           std::to_string(width) + "\n\tHeight: " +
           std::to_string(height) + "\n\tMip levels: " +
           std::to_string(mipLevels));

    auto *texture = new Texture();
    {
        texture->m_width          = width;
        texture->m_height         = height;
        texture->m_mipLevels      = mipLevels;
        texture->m_format         = format;
        texture->m_device         = device;
        texture->m_canBeDestroyed = true;
    }

    auto stagingBuffer = StagingBuffer::Create(device, (void*)pixels, texture->m_width, texture->m_height);

    //!=================================================================================================================

    texture->m_image = Tools::CreateImage(
            device,
            width, height,
            texture->m_mipLevels,
            texture->m_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            //VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &texture->m_deviceMemory, false);

    auto copyCmd = Types::CmdBuffer::BeginSingleTime(device, pool);

    Tools::TransitionImageLayout(copyCmd, texture->m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->m_mipLevels);
    Tools::CopyBufferToImage(copyCmd, *stagingBuffer, texture->m_image, width, height);
    if (mipLevels == 1) {
        Tools::TransitionImageLayout(copyCmd, texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture->m_mipLevels);
        texture->m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else
        if (!GenerateMipmaps(texture, copyCmd)) {
            VK_ERROR("Texture::Load() : failed to generate mipmaps!");
            return nullptr;
        }

    //!=================================================================================================================

    copyCmd->Destroy();
    copyCmd->Free();

    stagingBuffer->Destroy();
    stagingBuffer->Free();

    //!=================================================================================================================

    texture->m_view = Tools::CreateImageView(
            *device,
            texture->m_image,
            texture->m_format,
            texture->m_mipLevels,
            VK_IMAGE_ASPECT_COLOR_BIT);
    if (texture->m_view == VK_NULL_HANDLE) {
        VK_ERROR("Texture::Load() : failed to create image view!");
        return nullptr;
    }

    //!=================================================================================================================

    texture->m_sampler = Tools::CreateSampler(
            device,
            texture->m_mipLevels,
            filter, filter,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_COMPARE_OP_NEVER);
    if (texture->m_sampler == VK_NULL_HANDLE) {
        VK_ERROR("Texture::Load() : failed to create sampler image!");
        return nullptr;
    }

    //!=================================================================================================================

    return texture;
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
                subresourceRange);

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
                subresourceRange);

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
            subresourceRange);

    texture->m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return singleBuffer->End();
}


/*CompressBlockBC7(pixels, 32, cmpBuffer);
CompressBlockBC7(pixels + 16, 32, cmpBuffer + 16);
CompressBlockBC7(pixels + 128, 32, cmpBuffer + 32);
CompressBlockBC7(pixels + 144, 32, cmpBuffer + 48);*/
//unsigned char cmpBuffer[16 /*block size*/ * 16 /*block count*/] = { 0 };


/*uint32_t blockCount = (width / 4) * (height / 4);
auto* cmpBuffer = (uint8_t*)malloc(16 * blockCount * 4);

for (uint32_t col = 0; col < width / 4; col++) {
    for (uint32_t row = 0; row < height / 4; row++)
        CompressBlockBC7(pixels + col * 16 + row * 16 * width, 4 * width, cmpBuffer + (col * 16) + (row * width * 4));
}

pixels = cmpBuffer;*/
