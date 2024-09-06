//
// Created by Monika on 18.06.2023.
//

#ifndef SR_ENGINE_FRAMEBUFFERLAYER_H
#define SR_ENGINE_FRAMEBUFFERLAYER_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Image.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Complexes/FrameBufferAttachment.h>

namespace EvoVulkan::Complexes {
    class FrameBuffer;

    class FrameBufferLayer : public Tools::NonCopyable {
        using Attachment = std::unique_ptr<FrameBufferAttachment>;
        using Attachments = std::vector<Attachment>;
    public:
        explicit FrameBufferLayer(FrameBuffer* pFrameBuffer, uint32_t index, FrameBufferAttachment* pDepth);
        ~FrameBufferLayer() override;

    public:
        EVK_NODISCARD bool Initialize();

        EVK_NODISCARD Attachment& GetDepthAttachment() { return m_depthAttachment; }
        EVK_NODISCARD Attachments& GetColorAttachments() { return m_colorAttachments; }
        EVK_NODISCARD Attachments& GetResolveAttachments() { return m_resolveAttachments; }
        EVK_NODISCARD VkFramebuffer& GetFramebuffer() { return m_vkFrameBuffer; }

    private:
        FrameBufferAttachment* m_depthArray = nullptr;
        Attachments m_colorAttachments;
        Attachments m_resolveAttachments;
        Attachment m_depthAttachment;
        VkFramebuffer m_vkFrameBuffer = VK_NULL_HANDLE;
        FrameBuffer* m_frameBuffer = nullptr;
        uint32_t m_index = 0;

    };
}

#endif //SR_ENGINE_FRAMEBUFFERLAYER_H
