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
        bool depth)
{
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
    pMultiSample->m_depthEnabled  = depth;

    if (!pMultiSample->ReCreate(w, h)) {
        VK_ERROR("MultisampleTarget::Create() : failed to re-create multisample!");
        return nullptr;
    }

    return pMultiSample;
}

bool EvoVulkan::Types::MultisampleTarget::ReCreate(uint32_t w, uint32_t h) {
    Destroy();

    m_width = w;
    m_height = h;

    auto&& imageCI = Types::ImageCreateInfo(
        m_device, m_allocator, m_width, m_height,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        m_sampleCount
    );

    //! ----------------------------------- Color target -----------------------------------

    if (m_sampleCount > 1) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(*m_device, &deviceProperties);

        if (!((deviceProperties.limits.framebufferColorSampleCounts >= m_device->GetMSAASamples()) && (deviceProperties.limits.framebufferDepthSampleCounts >= m_device->GetMSAASamples()))) {
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

            m_resolves[i].m_view = Tools::CreateImageView(*m_device, m_resolves[i].m_image, m_formats[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);

            if (m_resolves[i].m_view == VK_NULL_HANDLE) {
                VK_ERROR("MultisampleTarget::ReCreate() : failed to create resolve image view!");
                return false;
            }
        }
    }

    //! ----------------------------------- Depth target -----------------------------------

    if (m_depthEnabled) {
        imageCI.format = m_swapchain->GetDepthFormat();
        imageCI.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        /// imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        if (!(m_depth.m_image = Types::Image::Create(imageCI)).Valid()) {
            VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image!");
            return false;
        }

        /// ставим барьер памяти, чтобы можно было использовать в шейдерах
        ///{
        ///    auto&& copyCmd = EvoVulkan::Types::CmdBuffer::BeginSingleTime(m_device, m_cmdPool);

        ///    EvoVulkan::Tools::TransitionImageLayoutEx(
        ///        copyCmd,
        ///        m_depth.m_image,
        ///        VK_IMAGE_LAYOUT_UNDEFINED,
        ///        VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
        ///        1, /** mip levels */
        ///        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
        ///    );

        ///    delete copyCmd;
        ///}

        m_depth.m_view = Tools::CreateImageView(*m_device, m_depth.m_image, m_swapchain->GetDepthFormat(), 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
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
        /// m_countResolves = dont touch
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
