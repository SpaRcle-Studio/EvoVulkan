//
// Created by Nikita on 11.06.2021.
//

#include <EvoVulkan/Memory/Allocator.h>
#include <EvoVulkan/Types/MultisampleTarget.h>
#include <EvoVulkan/Tools/VulkanTools.h>

EvoVulkan::Types::MultisampleTarget *EvoVulkan::Types::MultisampleTarget::Create(
    EvoVulkan::Types::Device *device,
    Memory::Allocator* allocator,
    Types::CmdPool* cmdPool,
    Swapchain* swapchain,
    uint32_t w, uint32_t h,
    const std::vector<VkFormat>& formats,
    uint8_t sampleCount,
    uint32_t layersCount,
    VkImageAspectFlags depthAspect,
    VkFormat depthFormat
) {
    if (sampleCount == 0) {
        sampleCount = device->GetMSAASamplesCount();
    }

    auto&& pMultiSample = new MultisampleTarget();

    pMultiSample->m_device        = device;
    pMultiSample->m_allocator     = allocator;
    pMultiSample->m_swapchain     = swapchain;
    pMultiSample->m_cmdPool       = cmdPool;
    pMultiSample->m_countResolves = formats.size();
    pMultiSample->m_formats       = formats;
    pMultiSample->m_sampleCount   = sampleCount;
    pMultiSample->m_depthAspect   = depthAspect;
    pMultiSample->m_depthFormat   = depthFormat;
    pMultiSample->m_layersCount   = layersCount;

    if (!pMultiSample->ReCreate(w, h)) {
        VK_ERROR("MultisampleTarget::Create() : failed to re-create multisample!");
        return nullptr;
    }

    return pMultiSample;
}

bool EvoVulkan::Types::MultisampleTarget::ReCreate(uint32_t width, uint32_t height) {
    VK_LOG("MultisampleTarget::ReCreate() : re-create multisample..."
           "\n\tWidth: " + std::to_string(width) + "\n\tHeight: " + std::to_string(height)
    );

    Destroy();

    m_width = width;
    m_height = height;

    auto&& imageCI = Types::ImageCreateInfo(
        m_device, m_allocator, m_width, m_height, 1,
        VK_FORMAT_UNDEFINED /** format */,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT /** usage */,
        m_sampleCount,
        false /** cpu usage */,
        1 /** mip levels */,
        m_layersCount,
        VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM
    );

    //! ----------------------------------- Color resolve target -----------------------------------

    if (m_sampleCount > 1 && m_countResolves > 0) {
        if (!m_device->IsMultiSamplingEnabled()) {
            VK_ERROR("MultisampleTarget::ReCreate() : multisampling is unsupported!");
            return false;
        }

        m_resolves = (Image*)malloc(sizeof(Image) * m_countResolves);

        for (uint32_t i = 0; i < m_countResolves; ++i) {
            imageCI.format = m_formats[i];

            if (!(m_resolves[i].m_image = Types::Image::Create(imageCI)).Valid()) {
                VK_ERROR("MultisampleTarget::ReCreate() : failed to create resolve image!");
                return false;
            }

            m_resolves[i].m_view = Tools::CreateImageView(
                *m_device,
                m_resolves[i].m_image,
                m_formats[i],
                1 /** mip levels */,
                VK_IMAGE_ASPECT_COLOR_BIT,
                m_layersCount,
                0,
                m_layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D
            );

            if (m_resolves[i].m_view == VK_NULL_HANDLE) {
                VK_ERROR("MultisampleTarget::ReCreate() : failed to create resolve image view!");
                return false;
            }
        }
    }

    //! ----------------------------------- Depth target -----------------------------------

    if (m_depthAspect != EvoVulkan::Tools::Initializers::EVK_IMAGE_ASPECT_NONE && m_depthFormat != VK_FORMAT_UNDEFINED) {
        imageCI.format = m_depthFormat;
        imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        if (!(m_depth.m_image = Types::Image::Create(imageCI)).Valid()) {
            VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image!");
            return false;
        }

        m_depth.m_view = Tools::CreateImageView(
            *m_device,
            m_depth.m_image,
            m_depthFormat,
            1 /** mip levels */,
            m_depthAspect,
            m_layersCount,
            0,
            m_layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D
        );

        if (m_depth.m_view == VK_NULL_HANDLE) {
            VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image view!");
            return false;
        }
    }

    return true;
}

void EvoVulkan::Types::MultisampleTarget::Destroy() {
    /// Destroy MSAA target
    if (m_resolves) {
        for (uint32_t i = 0; i < m_countResolves; ++i) {
            if (m_resolves[i].m_image && m_resolves[i].m_view && m_resolves[i].m_image.Valid()) {
                vkDestroyImageView(*m_device, m_resolves[i].m_view, nullptr);
                m_allocator->FreeImage(m_resolves[i].m_image);
                m_resolves[i].m_view = VK_NULL_HANDLE;
            }
        }
        free(m_resolves);
        m_resolves = nullptr;
        /// m_countResolves = don't touch
    }

    if (m_depth.m_image && m_depth.m_view && m_depth.m_image.Valid()) {
        vkDestroyImageView(*m_device, m_depth.m_view, nullptr);
        m_allocator->FreeImage(m_depth.m_image);
        m_depth.m_view = VK_NULL_HANDLE;
    }
}

VkImageView EvoVulkan::Types::MultisampleTarget::GetResolve(const uint32_t &id) const noexcept {
    if (m_sampleCount <= 1) {
        return VK_NULL_HANDLE;
    }

    return m_resolves[id].m_view;
}

void EvoVulkan::Types::MultisampleTarget::SetSampleCount(uint8_t sampleCount) {
    m_sampleCount = sampleCount;
}
