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
        EvoVulkan::Memory::Allocator* pAllocator,
        EvoVulkan::Types::CmdPool* pPool,
        FrameBufferAttachment* pImageArray,
        VkFormat format,
        VkImageAspectFlags aspect,
        VkExtent2D imageSize,
        uint32_t samplesCount,
        uint32_t layersCount,
        uint32_t layer
    ) {
        auto&& pFBOAttachment = std::make_unique<FrameBufferAttachment>();

        pFBOAttachment->m_format = format;
        pFBOAttachment->m_device = pAllocator->GetDevice();
        pFBOAttachment->m_allocator = pAllocator;

        if (pImageArray) {
            pFBOAttachment->m_image = pImageArray->GetImage().Copy();
            pFBOAttachment->m_weakImage = true;
        }
        else {
            auto&& imageCI = Types::ImageCreateInfo(
                pAllocator->GetDevice(), pAllocator, imageSize.width, imageSize.height,
                format,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT /** usage */,
                samplesCount,
                false /** cpu usage */,
                1 /** mip levels */,
                layersCount,
                VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM
            );

            if (!(pFBOAttachment->m_image = Types::Image::Create(imageCI)).Valid()) {
                VK_ERROR("FrameBufferAttachment::CreateDepthAttachment() : failed to create depth image!");
                return nullptr;
            }

            {
                auto&& copyCmd = EvoVulkan::Types::CmdBuffer::BeginSingleTime(pAllocator->GetDevice(), pPool);

                EvoVulkan::Tools::TransitionImageLayoutEx(
                    copyCmd,
                    pFBOAttachment->m_image,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                    1, // mip levels
                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
                    layersCount
                );

                delete copyCmd;
            }
        }

        pFBOAttachment->m_view = Tools::CreateImageView(
            *pAllocator->GetDevice(),
            pFBOAttachment->m_image,
            format,
            1 /** mip levels */,
            aspect,
            layersCount,
            layer,
            layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D
        );

        if (pFBOAttachment->m_view == VK_NULL_HANDLE) {
            VK_ERROR("FrameBufferAttachment::CreateDepthAttachment() : failed to create depth image view!");
            return nullptr;
        }

        return std::move(pFBOAttachment);
    }

    std::unique_ptr<FrameBufferAttachment> FrameBufferAttachment::CreateResolveAttachment(
        EvoVulkan::Memory::Allocator* pAllocator,
        VkFormat format,
        VkExtent2D imageSize,
        uint32_t samplesCount,
        uint32_t layersCount,
        uint32_t layer
    ) {
        auto&& pFBOAttachment = std::make_unique<FrameBufferAttachment>();

        pFBOAttachment->m_format = format;
        pFBOAttachment->m_device = pAllocator->GetDevice();
        pFBOAttachment->m_allocator = pAllocator;

        auto&& imageCI = Types::ImageCreateInfo(
                pAllocator->GetDevice(), pAllocator, imageSize.width, imageSize.height,
            format,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT /** usage */,
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

        pFBOAttachment->m_view = Tools::CreateImageView(
            *pAllocator->GetDevice(),
            pFBOAttachment->m_image,
            format,
            1 /** mip levels */,
            VK_IMAGE_ASPECT_COLOR_BIT,
            layersCount /** layers count */,
            layer /** layer index */,
            layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D
        );

        if (pFBOAttachment->m_view == VK_NULL_HANDLE) {
            VK_ERROR("FrameBufferAttachment::CreateResolveAttachment() : failed to create resolve image view!");
            return nullptr;
        }

        return std::move(pFBOAttachment);
    }

    std::unique_ptr<FrameBufferAttachment> FrameBufferAttachment::CreateColorAttachment(
        EvoVulkan::Memory::Allocator* pAllocator,
        EvoVulkan::Types::CmdPool* pool,
        VkFormat format,
        VkImageUsageFlags usage,
        VkExtent2D imageSize,
        uint32_t samplesCount,
        uint32_t layersCount,
        uint32_t layer
    ) {
        auto&& pFBOAttachment = std::make_unique<FrameBufferAttachment>();

        pFBOAttachment->m_format = format;
        pFBOAttachment->m_device = pAllocator->GetDevice();
        pFBOAttachment->m_allocator = pAllocator;

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

        auto&& imageCI = EvoVulkan::Types::ImageCreateInfo(
            pAllocator->GetDevice(),
            pAllocator,
            imageSize.width,
            imageSize.height,
            format,
            usage,
            samplesCount,
            false /** cpu usage */,
            1 /** mip levels */,
            layersCount
        );

        pFBOAttachment->m_image = EvoVulkan::Types::Image::Create(imageCI);

        /// ставим барьер памяти, чтобы можно было использовать в шейдерах
        {
            auto&& pCopyCmd = EvoVulkan::Types::CmdBuffer::BeginSingleTime(pAllocator->GetDevice(), pool);

            EvoVulkan::Tools::TransitionImageLayout(
                pCopyCmd,
                pFBOAttachment->m_image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1 /** mip levels */,
                layersCount
            );

            delete pCopyCmd;
        }

        pFBOAttachment->m_view = EvoVulkan::Tools::CreateImageView(
            *pAllocator->GetDevice(),
            pFBOAttachment->m_image,
            format,
            1 /** mip levels */,
            aspectMask,
            layersCount,
            layer,
            layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D
        );

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
               m_device != VK_NULL_HANDLE &&
               m_format != VK_FORMAT_UNDEFINED;
    }
}