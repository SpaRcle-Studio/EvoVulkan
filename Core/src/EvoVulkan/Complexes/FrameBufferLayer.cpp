//
// Created by Monika on 18.06.2023.
//

#include <EvoVulkan/Complexes/FrameBufferLayer.h>
#include <EvoVulkan/Complexes/Framebuffer.h>

namespace EvoVulkan::Complexes {
    FrameBufferLayer::FrameBufferLayer(FrameBuffer* pFrameBuffer, uint32_t index, FrameBufferAttachment* pDepth)
        : m_frameBuffer(pFrameBuffer)
        , m_index(index)
        , m_depthArray(pDepth)
    { }

    FrameBufferLayer::~FrameBufferLayer() {
        if (m_vkFrameBuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(*m_frameBuffer->GetDevice(), m_vkFrameBuffer, nullptr);
            m_vkFrameBuffer = VK_NULL_HANDLE;
        }
    }

    bool FrameBufferLayer::Initialize() {
        auto&& colorFormats = m_frameBuffer->GetColorFormats();
        m_colorAttachments.resize(colorFormats.size());
        m_resolveAttachments.resize(m_frameBuffer->IsMultisampleEnabled() * colorFormats.size());

        /// =============================================== color targets ==============================================

        for (uint32_t i = 0; i < colorFormats.size(); ++i) {
            const VkImageUsageFlags usageFlags =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                (m_frameBuffer->GetFeatures().colorShaderRead ? VK_IMAGE_USAGE_SAMPLED_BIT : 0) |
                (m_frameBuffer->GetFeatures().transferSrcColor ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0) |
                (m_frameBuffer->GetFeatures().transferDstColor ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0)
            ;

            m_colorAttachments[i] = FrameBufferAttachment::CreateColorAttachment(
                m_frameBuffer,
                colorFormats[i],
                usageFlags,
                1 /** layers count */,
                m_index /** layer index */
            );

            if (!m_colorAttachments[i]->Ready()) {
                VK_ERROR("FrameBufferLayer::Initialize() : failed to create color attachment!");
                return false;
            }
        }

        /// ========================================== color resolves targets ==========================================

        if (m_frameBuffer->IsMultisampleEnabled() && !colorFormats.empty()) {
            for (uint32_t i = 0; i < colorFormats.size(); ++i) {
                m_resolveAttachments[i] = FrameBufferAttachment::CreateResolveAttachment(
                    m_frameBuffer,
                    colorFormats[i],
                    1 /** layers count */,
                    m_index /** layer index */
                );

                if (!m_resolveAttachments[i]->Ready()) {
                    VK_ERROR("FrameBufferLayer::Initialize() : failed to create color resolve attachment!");
                    return false;
                }
            }
        }

        /// =============================================== depth target ===============================================

        auto&& depthAspect = m_frameBuffer->GetDepthAspect();
        auto&& depthFormat = m_frameBuffer->GetDepthFormat();

        if (depthAspect != EvoVulkan::Tools::Initializers::EVK_IMAGE_ASPECT_NONE && depthFormat != VK_FORMAT_UNDEFINED) {
            m_depthAttachment = FrameBufferAttachment::CreateDepthAttachment(
                m_frameBuffer,
                m_depthArray,
                depthFormat,
                depthAspect,
                1 /** layers count */,
                m_index /** layer index */
            );

            if (!m_depthAttachment->Ready()) {
                VK_ERROR("FrameBufferLayer::Initialize() : failed to create depth attachment!");
                return false;
            }
        }

        return true;
    }
}
