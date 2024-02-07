//
// Created by Monika on 18.06.2023.
//

#include <EvoVulkan/Complexes/FrameBufferAttachment.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/Image.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanTools.h>

namespace EvoVulkan::Complexes {
    std::unique_ptr<FrameBufferAttachment> FrameBufferAttachment::CreateDepthAttachment(
        EvoVulkan::Complexes::FrameBuffer* pFrameBuffer,
        FrameBufferAttachment* pImageArray,
        VkFormat format,
        VkImageAspectFlags aspect,
        uint32_t layersCount,
        uint32_t layer
    ) {
        auto&& pFBOAttachment = std::make_unique<FrameBufferAttachment>();

        pFBOAttachment->m_device = pFrameBuffer->GetDevice();
        pFBOAttachment->m_allocator = pFrameBuffer->GetAllocator();

        if (pImageArray) {
            pFBOAttachment->m_image = pImageArray->GetImage().Copy();
            pFBOAttachment->m_weakImage = true;
        }
        else {
            auto&& imageSize = pFrameBuffer->GetExtent2D();
            auto&& samplesCount = pFrameBuffer->GetSampleCount();

            const VkImageUsageFlags usage =
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                (pFrameBuffer->GetFeatures().depthShaderRead ? VK_IMAGE_USAGE_SAMPLED_BIT : 0) |
                (pFrameBuffer->GetFeatures().depthTransferSrc ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) |
                (pFrameBuffer->GetFeatures().depthTransferDst ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0)
            ;

            auto&& imageCI = Types::ImageCreateInfo(
                pFrameBuffer->GetAllocator(), pFrameBuffer->GetCmdPool(), imageSize.width, imageSize.height, 1,
                aspect, format, usage, samplesCount,
                false /** cpu usage */,
                1 /** mip levels */,
                layersCount,
                VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM
            );

            if (!(pFBOAttachment->m_image = Types::Image::Create(imageCI)).Valid()) {
                VK_ERROR("FrameBufferAttachment::CreateDepthAttachment() : failed to create depth image!");
                return nullptr;
            }

            const VkImageLayout layout = Tools::FindDepthFormatLayout(pFrameBuffer->GetDepthAspect(), pFrameBuffer->GetFeatures().depthShaderRead, false);

            if (!pFBOAttachment->m_image.TransitionImageLayout(layout, VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT)) {
                VK_ERROR("FrameBufferAttachment::CreateDepthAttachment() : failed to transition depth image layout!");
                return nullptr;
            }
        }

        pFBOAttachment->m_view = Tools::CreateImageView(pFBOAttachment->m_image, layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D, layer);
        if (pFBOAttachment->m_view == VK_NULL_HANDLE) {
            VK_ERROR("FrameBufferAttachment::CreateDepthAttachment() : failed to create depth image view!");
            return nullptr;
        }

        return std::move(pFBOAttachment);
    }

    std::unique_ptr<FrameBufferAttachment> FrameBufferAttachment::CreateResolveAttachment(
        EvoVulkan::Complexes::FrameBuffer* pFrameBuffer,
        VkFormat format,
        uint32_t layersCount,
        uint32_t layer
    ) {
        auto&& pFBOAttachment = std::make_unique<FrameBufferAttachment>();

        pFBOAttachment->m_device = pFrameBuffer->GetDevice();
        pFBOAttachment->m_allocator = pFrameBuffer->GetAllocator();

        auto&& imageSize = pFrameBuffer->GetExtent2D();
        auto&& samplesCount = pFrameBuffer->GetSampleCount();

        auto&& imageCI = Types::ImageCreateInfo(
            pFrameBuffer->GetAllocator(), pFrameBuffer->GetCmdPool(), imageSize.width, imageSize.height, VK_IMAGE_ASPECT_COLOR_BIT, 1,
            format,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT/** usage */,
            samplesCount,
            false /** cpu usage */,
            1 /** mip levels */,
            layersCount,
            VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM
        );

        if (!(pFBOAttachment->m_image = Types::Image::Create(imageCI)).Valid()) {
            VK_ERROR("FrameBufferAttachment::CreateResolveAttachment() : failed to create resolve image!");
            return nullptr;
        }

        if (!pFBOAttachment->m_image.TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            VK_ERROR("FrameBufferAttachment::CreateDepthAttachment() : failed to transition depth image layout!");
            return nullptr;
        }

        pFBOAttachment->m_view = Tools::CreateImageView(pFBOAttachment->m_image, layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D, layer);
        if (pFBOAttachment->m_view == VK_NULL_HANDLE) {
            VK_ERROR("FrameBufferAttachment::CreateResolveAttachment() : failed to create resolve image view!");
            return nullptr;
        }

        return std::move(pFBOAttachment);
    }

    std::unique_ptr<FrameBufferAttachment> FrameBufferAttachment::CreateColorAttachment(
        EvoVulkan::Complexes::FrameBuffer* pFrameBuffer,
        VkFormat format,
        VkImageUsageFlags usage,
        uint32_t layersCount,
        uint32_t layer
    ) {
        auto&& pFBOAttachment = std::make_unique<FrameBufferAttachment>();

        pFBOAttachment->m_device = pFrameBuffer->GetDevice();
        pFBOAttachment->m_allocator = pFrameBuffer->GetAllocator();

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;

        if (usage & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            aspectMask |= VK_IMAGE_USAGE_SAMPLED_BIT;

        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        if (aspectMask == VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM) {
            VK_ERROR("FrameBufferAttachment::CreateAttachment() : incorrect image usage!");
            return {};
        }

        auto&& imageSize = pFrameBuffer->GetExtent2D();
        const uint8_t samplesCount = 1;

        auto&& imageCI = EvoVulkan::Types::ImageCreateInfo(
            pFrameBuffer->GetAllocator(),
            pFrameBuffer->GetCmdPool(),
            imageSize.width,
            imageSize.height,
            1,
            aspectMask,
            format,
            usage,
            samplesCount,
            false /** cpu usage */,
            1 /** mip levels */,
            layersCount
        );

        pFBOAttachment->m_image = EvoVulkan::Types::Image::Create(imageCI);

        const VkImageLayout layout = pFrameBuffer->GetFeatures().colorShaderRead ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        /// ставим барьер памяти, чтобы можно было использовать в шейдерах
        pFBOAttachment->m_image.TransitionImageLayout(layout);

        pFBOAttachment->m_view = EvoVulkan::Tools::CreateImageView(pFBOAttachment->m_image, layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D, layer);

        return std::move(pFBOAttachment);
    }

    FrameBufferAttachment::~FrameBufferAttachment() {
        if (m_view) {
            vkDestroyImageView(*m_device, m_view, nullptr);
            m_view = VK_NULL_HANDLE;
        }

        if (m_image.Valid() && !m_weakImage) {
            m_allocator->FreeImage(m_image);
        }

        m_device = nullptr;
        m_allocator = nullptr;
    }

    bool FrameBufferAttachment::Ready() const {
        return m_image.Valid() &&
               m_view   != VK_NULL_HANDLE &&
               m_device != VK_NULL_HANDLE;
    }
}