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

namespace EvoVulkan::Types {
    struct Texture {
        VkSampler m_sampler;
        VkImage m_image;
        VkImageLayout m_imageLayout;
        VkDeviceMemory m_deviceMemory;
        VkImageView m_view;
        uint32_t m_width, m_height;
        uint32_t m_mipLevels;

        static Texture* LoadAutoMip(
                const Device *device,
                const CmdPool *pool,
                const unsigned char *pixels,
                uint32_t width,
                uint32_t height,
                uint8_t channels)
        {
            return Load(device, pool, pixels, width, height,
                        static_cast<uint32_t>(std::floor(std::log2(max(width, height)))) + 1, channels);
        }

        static Texture* Load(
                const Device *device,
                const CmdPool *pool, const unsigned char *pixels,
                uint32_t width, uint32_t height,
                uint32_t mipLevels,
                uint8_t channels) {
            channels += 1;

            VK_LOG("Texture::Load() : loading new texture... \n\tWidth: " +
                   std::to_string(width) + "\n\tHeight: " +
                   std::to_string(height) + "\n\tChannels: " +
                   std::to_string((int) channels));

            auto *texture = new Texture();
            {
                texture->m_width     = width;
                texture->m_height    = height;
                texture->m_mipLevels = mipLevels;
            }

            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

            uint64_t imageSize = width * height * channels;

            //!=========================================================================================================

            VkDeviceMemory stagingBufferMemory;
            VkBuffer stagingBuffer = Tools::CreateBuffer(
                    device,
                    imageSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBufferMemory);

            void* data;
            vkMapMemory(*device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(*device, stagingBufferMemory);

            texture->m_image = Tools::CreateImage(
                    device,
                    width, height,
                    mipLevels, format,
                    VK_IMAGE_TILING_OPTIMAL,
                    //VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &texture->m_deviceMemory);

            auto copyCmd = Types::CmdBuffer::BeginSingleTime(device, pool);

            Tools::TransitionImageLayout(copyCmd, texture->m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
            Tools::CopyBufferToImage(copyCmd, stagingBuffer, texture->m_image, width, height);
            Tools::TransitionImageLayout(copyCmd, texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

            copyCmd->Destroy();
            copyCmd->Free();

            texture->m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            //!=========================================================================================================

            vkDestroyBuffer(*device, stagingBuffer, nullptr);
            vkFreeMemory(*device, stagingBufferMemory, nullptr);

            //!=========================================================================================================

            texture->m_view = Tools::CreateImageView(*device, texture->m_image, format, mipLevels);
            if (texture->m_view == VK_NULL_HANDLE) {
                VK_ERROR("Texture::Load() : failed to create image view!");
                return nullptr;
            }

            //!=========================================================================================================

            texture->m_sampler = Tools::CreateSampler(
                    device,
                    mipLevels,
                    VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                    VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    VK_COMPARE_OP_NEVER);
            if (texture->m_sampler == VK_NULL_HANDLE) {
                VK_ERROR("Texture::Load() : failed to create sampler image!");
                return nullptr;
            }

            //!=========================================================================================================

            return texture;
        }

        /*
         *
         */

        /*
        ///* \note mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
        static Texture* Load(
                const Device *device,
                const CmdPool *pool, const unsigned char *pixels,
                uint32_t width, uint32_t height,
                uint32_t mipLevels,
                uint8_t channels)
        {
            VK_LOG("Texture::Load() : loading new texture... \n\tWidth: " +
                std::to_string(width) + "\n\tHeight: " +
                std::to_string(height) + "\n\tChannels: " +
                std::to_string((int)channels));

            auto *texture = new Texture();
            {
                texture->m_width = width;
                texture->m_height = height;
                texture->m_mipLevels = mipLevels;
            }

            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            if (channels == 4) // TODO!
                format = VK_FORMAT_R8G8B8A8_UNORM;

            uint64_t imageSize = width * height * (channels + 1); // * ((int)channels - 2) + 2500
            //uint64_t len = strlen((char*)pixels);

            //std::cout << imageSize << std::endl;
            //std::cout << len << std::endl;

            auto copyCmd = Types::CmdBuffer::BeginSingleTime(device, pool);
            auto blitCmd = Types::CmdBuffer::BeginSingleTime(device, pool);
            if (!copyCmd || !blitCmd) {
                VK_ERROR("Texture::Load() : failed to begin single time command buffers!");
                return nullptr;
            }

            //!=========================================================================================================

            VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
            VkBuffer stagingBuffer = Tools::CreateBuffer(
                    device,
                    imageSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingMemory);
            if (stagingBuffer == VK_NULL_HANDLE || stagingMemory == VK_NULL_HANDLE) {
                VK_ERROR("Texture::Load() : failed to create staging buffer!");
                return nullptr;
            }

            //!=========================================================================================================

            // Copy texture data into host local staging buffer
            uint8_t *data;
            if (vkMapMemory(*device, stagingMemory, 0, imageSize, 0, (void **) &data) != VK_SUCCESS) {
                VK_ERROR("Texture::Load() : failed to map vulkan memory!");
                return nullptr;
            }
            memcpy(data, pixels, imageSize);
            vkUnmapMemory(*device, stagingMemory);

            //!=========================================================================================================

            texture->m_image = Tools::CreateImage(
                    device,
                    width, height,
                    mipLevels, format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &texture->m_deviceMemory);

            if (!texture->m_image) {
                return nullptr;
            }

            //!=========================================================================================================

            VkBufferImageCopy buffer_copy_region = {};
            buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            buffer_copy_region.imageSubresource.mipLevel = 0;
            buffer_copy_region.imageSubresource.baseArrayLayer = 0;
            buffer_copy_region.imageSubresource.layerCount = 1;
            buffer_copy_region.imageExtent.width = width;
            buffer_copy_region.imageExtent.height = height;
            buffer_copy_region.imageExtent.depth = 1;
            vkCmdCopyBufferToImage(*copyCmd, stagingBuffer, texture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &buffer_copy_region);

            //!=======================================[Prepare base mip level]==========================================

            Tools::Insert::ImageMemoryBarrier(
                    *copyCmd,
                    texture->m_image,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

            //!======================================[Generating the mip-chain]=========================================

            for (int32_t i = 1; i < texture->m_mipLevels; i++) {
                VkImageBlit image_blit{};

                // Source
                image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_blit.srcSubresource.layerCount = 1;
                image_blit.srcSubresource.mipLevel   = i - 1;
                image_blit.srcOffsets[1].x           = int32_t(width >> (i - 1));
                image_blit.srcOffsets[1].y           = int32_t(height >> (i - 1));
                image_blit.srcOffsets[1].z           = 1;

                // Destination
                image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_blit.dstSubresource.layerCount = 1;
                image_blit.dstSubresource.mipLevel   = i;
                image_blit.dstOffsets[1].x           = int32_t(width >> i);
                image_blit.dstOffsets[1].y           = int32_t(height >> i);
                image_blit.dstOffsets[1].z           = 1;

                // Prepare current mip level as image blit destination
                Tools::Insert::ImageMemoryBarrier(
                        *blitCmd,
                        texture->m_image,
                        0,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        {VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)i, 1, 0, 1});

                vkCmdBlitImage(
                        *blitCmd,
                        texture->m_image,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        texture->m_image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &image_blit,
                        VK_FILTER_LINEAR);

                Tools::Insert::ImageMemoryBarrier(
                        *blitCmd,
                        texture->m_image,
                        VK_ACCESS_TRANSFER_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        {VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)i, 1, 0, 1});
            }

            //!=========================================================================================================

            Tools::Insert::ImageMemoryBarrier(
                    *blitCmd,
                    texture->m_image,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, texture->m_mipLevels, 0, 1});

            //!=========================================================================================================

            texture->m_view = Tools::CreateImageView(*device, texture->m_image, format, mipLevels);
            if (texture->m_view == VK_NULL_HANDLE) {
                VK_ERROR("Texture::Load() : failed to create image view!");
                return nullptr;
            }

            //!=========================================================================================================

            texture->m_sampler = Tools::CreateSampler(
                    device,
                    mipLevels,
                    VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                    VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    VK_COMPARE_OP_NEVER);
            if (texture->m_sampler == VK_NULL_HANDLE) {
                VK_ERROR("Texture::Load() : failed to create sampler image!");
                return nullptr;
            }

            //!=========================================================================================================

            copyCmd->Destroy();
            copyCmd->Free();

            blitCmd->Destroy();
            blitCmd->Free();

            return texture;
        }*/
    };
}

#endif //EVOVULKAN_TEXTURE_H
