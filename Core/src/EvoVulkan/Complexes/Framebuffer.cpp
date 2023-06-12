//
// Created by Nikita on 13.06.2021.
//

#include <EvoVulkan/Complexes/Framebuffer.h>
#include <EvoVulkan/Types/CmdBuffer.h>
#include <EvoVulkan/Types/CmdBuffer.h>

namespace EvoVulkan::Complexes {
    static FrameBufferAttachment CreateAttachment(
        EvoVulkan::Types::Device* device,
        EvoVulkan::Memory::Allocator* allocator,
        EvoVulkan::Types::CmdPool* pool,
        VkFormat format,
        VkImageUsageFlags usage,
        VkExtent2D imageSize,
        uint32_t samplesCount,
        uint32_t layersCount
    ) {
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;

        if (usage & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
            aspectMask |= VK_IMAGE_USAGE_SAMPLED_BIT;

        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        if (aspectMask == VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM) {
            VK_ERROR("Types::CreateAttachment() : incorrect image usage!");
            return {};
        }

        EvoVulkan::Complexes::FrameBufferAttachment FBOAttachment = {};

        auto&& imageCI = EvoVulkan::Types::ImageCreateInfo(
            device,
            allocator,
            imageSize.width,
            imageSize.height,
            format,
            usage,
            samplesCount,
            false /** cpu usage */,
            1 /** mip levels */,
            layersCount
        );

        FBOAttachment.m_image = EvoVulkan::Types::Image::Create(imageCI);

        /// ставим барьер памяти, чтобы можно было использовать в шейдерах
        {
            auto&& pCopyCmd = EvoVulkan::Types::CmdBuffer::BeginSingleTime(device, pool);

            EvoVulkan::Tools::TransitionImageLayout(
                pCopyCmd,
                FBOAttachment.m_image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1 /** mip levels */,
                layersCount
            );

            delete pCopyCmd;
        }

        FBOAttachment.m_view = EvoVulkan::Tools::CreateImageView(
            *device,
            FBOAttachment.m_image,
            format,
            1 /** mip levels */,
            aspectMask,
            layersCount,
            layersCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D
        );

        FBOAttachment.m_format = format;
        FBOAttachment.m_device = device;
        FBOAttachment.m_allocator = allocator;

        return std::move(FBOAttachment);
    }

    FrameBuffer::~FrameBuffer() {
        DeInitialize();

        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(*m_device, m_semaphore, nullptr);
            m_semaphore = VK_NULL_HANDLE;
        }

        if (m_cmdBuff) {
            delete m_cmdBuff;
            m_cmdBuff = nullptr;
        }

        if (m_renderPass.IsReady()) {
            Types::DestroyRenderPass(m_device, &m_renderPass);
        }

        m_device    = nullptr;
        m_swapchain = nullptr;
        m_cmdPool   = nullptr;
    }

    FrameBuffer* FrameBuffer::Create(
        Types::Device* device,
        EvoVulkan::Memory::Allocator* allocator,
        Core::DescriptorManager* manager,
        Types::Swapchain* swapchain,
        Types::CmdPool* pool,
        const std::vector<VkFormat>& colorAttachments,
        uint32_t width, uint32_t height,
        uint32_t arrayLayers,
        float_t scale,
        uint8_t samplesCount,
        VkImageAspectFlags depthAspect,
        VkFormat depthFormat
    ) {
        if (scale <= 0.f) {
            VK_ERROR("Framebuffer::Create() : scale <= zero!");
            return nullptr;
        }

        if (arrayLayers <= 0 || arrayLayers > 256) {
            VK_ERROR("Framebuffer::Create() : invalid array layers!");
            return nullptr;
        }

        auto&& pFBO = new FrameBuffer();
        {
            pFBO->m_arrayLayers        = arrayLayers;
            pFBO->m_scale              = scale;
            pFBO->m_cmdPool            = pool;
            pFBO->m_device             = device;
            pFBO->m_allocator          = allocator;
            pFBO->m_descriptorManager  = manager;
            pFBO->m_swapchain          = swapchain;
            pFBO->m_countColorAttach   = colorAttachments.size();
            pFBO->m_attachFormats      = colorAttachments;
            pFBO->m_depthFormat        = depthFormat;
            pFBO->m_depthAspect        = depthAspect;
        }

        pFBO->SetSampleCount(samplesCount);

        auto&& semaphoreCI = Tools::Initializers::SemaphoreCreateInfo();
        if (vkCreateSemaphore(*device, &semaphoreCI, nullptr, &pFBO->m_semaphore) != VK_SUCCESS) {
            VK_ERROR("Framebuffer::Create() : failed to create vulkan semaphore!");
            return nullptr;
        }

        pFBO->m_cmdBuff = Types::CmdBuffer::Create(device, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        pFBO->m_cmdBufInfo = Tools::Initializers::CommandBufferBeginInfo();

        if (!pFBO->ReCreate(width, height)) {
            VK_ERROR("Framebuffer::Create() : failed to re-create framebuffer!");
            return nullptr;
        }

        return pFBO;
    }

    bool EvoVulkan::Complexes::FrameBuffer::ReCreate(uint32_t width, uint32_t height)  {
        DeInitialize();

        m_baseWidth  = width;
        m_baseHeight = height;

        m_width  = m_baseWidth  * m_scale;
        m_height = m_baseHeight * m_scale;

        if (m_width <= 0 || m_height <= 0) {
            VK_ERROR("Framebuffer::ReCreate() : invalid sizes!");
            return false;
        }

        if (!CreateRenderPass()) {
            VK_ERROR("Framebuffer::ReCreate() : failed to create render pass!");
            return false;
        }

        m_multisampleTarget = Types::MultisampleTarget::Create(
            m_device,
            m_allocator,
            m_cmdPool,
            m_swapchain,
            width, height,
            m_attachFormats,
            GetSampleCount(),
            m_arrayLayers,
            m_depthAspect,
            m_depthFormat
        );

        if (!m_multisampleTarget) {
            VK_ERROR("Framebuffer::ReCreate() : failed to create multisample target!");
            return false;
        }

        m_viewport = Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
        m_scissor  = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);

        if (!CreateColorAttachments() || !CreateFramebuffer() || !CreateSampler()) {
            VK_ERROR("Framebuffer::ReCreate() : failed to re-create frame buffer!");
            return false;
        }

        return true;
    }

    bool EvoVulkan::Complexes::FrameBuffer::CreateSampler()  {
        VkSamplerCreateInfo sampler = Tools::Initializers::SamplerCreateInfo();

        sampler.magFilter     = VK_FILTER_NEAREST;
        sampler.minFilter     = VK_FILTER_NEAREST;
        sampler.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV  = sampler.addressModeU;
        sampler.addressModeW  = sampler.addressModeU;
        sampler.mipLodBias    = 0.0f;
        sampler.maxAnisotropy = 1.0f;
        sampler.minLod        = 0.0f;
        sampler.maxLod        = 1.0f;
        sampler.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        if (vkCreateSampler(*m_device, &sampler, nullptr, &m_colorSampler) != VK_SUCCESS) {
            VK_ERROR("Framebuffer::CreateSampler() : failed to create vulkan sampler!");
            return false;
        }

        return true;
    }

    bool EvoVulkan::Complexes::FrameBuffer::CreateFramebuffer()  {
        VK_GRAPH("Framebuffer::CreateFramebuffer() : create vulkan framebuffer...");

        std::vector<VkImageView> attachments = {};

        /**
         *  0 - resolve
         *  1 - color
         *  2 - resolve
         *  3 - color
         *  ...
         *  n - depth
        */

        for (uint32_t i = 0; i < m_countColorAttach; ++i) {
            if (IsMultisampleEnabled()) {
                attachments.push_back(m_multisampleTarget->GetResolve(i));
            }

            attachments.push_back(m_attachments[i].m_view);
        }

        if (IsDepthEnabled()) {
            attachments.push_back(m_multisampleTarget->GetDepth());
        }

        /// VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {};
        /// framebufferAttachmentsCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;

        if (m_renderPass.m_countAttachments != attachments.size()) {
            VK_ERROR("Framebuffer::CreateFramebuffer() : incompatible attachments!"
                     "\n\tRender pass: " + std::to_string(m_renderPass.m_countAttachments) +
                     "\n\tFramebuffer: " + std::to_string(attachments.size())
            );
            return false;
        }

        VkFramebufferCreateInfo FBO_CI = {};
        FBO_CI.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        /// FBO_CI.pNext               = &framebufferAttachmentsCreateInfo;
        /// FBO_CI.flags               = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
        FBO_CI.renderPass              = m_renderPass.m_self;
        FBO_CI.pAttachments            = attachments.data();
        FBO_CI.attachmentCount         = static_cast<uint32_t>(attachments.size());
        FBO_CI.width                   = m_width;
        FBO_CI.height                  = m_height;
        FBO_CI.layers                  = 1;

        if (vkCreateFramebuffer(*m_device, &FBO_CI, nullptr, &m_framebuffer) != VK_SUCCESS || m_framebuffer == VK_NULL_HANDLE) {
            VK_ERROR("Framebuffer::CreateFramebuffer() : failed to create vulkan framebuffer!");
            return false;
        }

        return true;
    }

    bool EvoVulkan::Complexes::FrameBuffer::CreateRenderPass()  {
        const bool dirtySamples = m_currentSampleCount != m_sampleCount;

        m_currentSampleCount = m_sampleCount;

        if (m_renderPass.IsReady()) {
            if (dirtySamples) {
                Types::DestroyRenderPass(m_device, &m_renderPass);
            }
            else {
                return true;
            }
        }

        std::vector<VkAttachmentDescription> attachmentDescs = {};
        VkAttachmentDescription attachmentDesc = {};

        /// Init attachment properties
        for (uint32_t i = 0; i < m_countColorAttach + (IsDepthEnabled() ? 1 : 0); ++i) {
            attachmentDesc.samples = GetSampleCount();
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (i == m_countColorAttach) {
                attachmentDesc.format = m_depthFormat;
                attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

                // if (m_device->IsSeparateDepthStencilLayoutsSupported()) {
                //     if ((m_depthAspect & VK_IMAGE_ASPECT_DEPTH_BIT) && (m_depthAspect & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                //         /// уже задали
                //     }
                //     else if (m_depthAspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
                //         attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                //     }
                //     else if (m_depthAspect & VK_IMAGE_ASPECT_STENCIL_BIT) {
                //         attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
                //     }
                //     else {
                //         VK_ERROR("FrameBuffer::CreateRenderPass() : invalid depth aspect!");
                //         return false;
                //     }
                // }
            }
            else {
                attachmentDesc.format = m_attachFormats[i];
                attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            attachmentDescs.push_back(attachmentDesc);
        }

        std::vector<VkAttachmentReference> inputAttachments;

        m_renderPass = Types::CreateRenderPass(
            m_device,
            m_swapchain,
            attachmentDescs,
            inputAttachments,
            Tools::Convert::SampleCountToInt(GetSampleCount()),
            m_depthAspect,
            m_depthFormat
        );

        if (!m_renderPass.IsReady()) {
            VK_ERROR("Framebuffer::CreateRenderPass() : failed to create render pass!");
            return false;
        }

        /// clear values
        {
            m_clearValues.clear();

            for (uint32_t i = 0; i < m_countColorAttach * (IsMultisampleEnabled() ? 2 : 1); ++i) {
                m_clearValues.emplace_back(VkClearValue{.color = {{0.0, 0.0, 0.0, 1.0}}});
            }

            if (IsDepthEnabled()) {
                m_clearValues.emplace_back(VkClearValue{.depthStencil = {1.0, 0}});
            }

            m_countClearValues = m_clearValues.size();
        }

        return true;
    }

    bool EvoVulkan::Complexes::FrameBuffer::CreateColorAttachments()  {
        if (m_countColorAttach == 0) {
            /// только буфер глубины, цвет не нужен
            return true;
        }

        m_attachments = (FrameBufferAttachment*)malloc(sizeof(FrameBufferAttachment) * m_countColorAttach);

        if (!m_attachments) {
            VK_ERROR("Framebuffer::CreateAttachments() : failed to allocate memory!");
            return false;
        }

        for (uint32_t i = 0; i < m_countColorAttach; ++i) {
            m_attachments[i].Init();
        }

        for (uint32_t i = 0; i < m_countColorAttach; ++i) {
            VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            m_attachments[i] = CreateAttachment(
                m_device,
                m_allocator,
                m_cmdPool,
                m_attachFormats[i],
                usageFlags,
                VkExtent2D { m_width, m_height },
                1 /** samples count */,
                m_arrayLayers
            );

            if (!m_attachments[i].Ready()) {
                VK_ERROR("Framebuffer::CreateAttachments() : failed to create color attachment!");
                return false;
            }
        }

        return true;
    }

    EvoVulkan::Types::Texture *EvoVulkan::Complexes::FrameBuffer::AllocateDepthTextureReference() {
        if (!IsDepthEnabled()) {
            return nullptr;
        }

        auto&& texture = new EvoVulkan::Types::Texture();

        texture->m_view              = m_multisampleTarget->GetDepth();
        texture->m_image             = m_multisampleTarget->GetDepthImage().Copy();
        texture->m_format            = m_depthFormat; /// TODO: why used VK_FORMAT_B8G8R8A8_UNORM?
        texture->m_descriptorManager = m_descriptorManager;
        texture->m_sampler           = m_colorSampler;
        texture->m_imageLayout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texture->m_device            = m_device;
        texture->m_pool              = m_cmdPool;
        texture->m_allocator         = m_allocator;
        texture->m_width             = m_width;
        texture->m_height            = m_height;
        texture->m_canBeDestroyed    = false;
        texture->m_mipLevels         = 1;

        /// make a texture descriptor
        texture->m_descriptor = {
                texture->m_sampler,
                texture->m_view,
                texture->m_imageLayout
        };

        /// texture->RandomizeSeed();

        return texture;
    }

    std::vector<EvoVulkan::Types::Texture*> EvoVulkan::Complexes::FrameBuffer::AllocateColorTextureReferences() {
        auto references = std::vector<EvoVulkan::Types::Texture*>();

        for (uint32_t i = 0; i < m_countColorAttach; ++i) {
            auto* texture = new EvoVulkan::Types::Texture();

            texture->m_view              = m_attachments[i].m_view;
            texture->m_image             = m_attachments[i].m_image.Copy();
            texture->m_format            = m_attachments[i].m_format;
            texture->m_descriptorManager = m_descriptorManager;
            texture->m_sampler           = m_colorSampler;
            texture->m_imageLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texture->m_device            = m_device;
            texture->m_pool              = m_cmdPool;
            texture->m_allocator         = m_allocator;
            texture->m_width             = m_width;
            texture->m_height            = m_height;
            texture->m_canBeDestroyed    = false;
            texture->m_mipLevels         = 1;

            //! make a texture descriptor
            texture->m_descriptor = {
                    texture->m_sampler,
                    texture->m_view,
                    texture->m_imageLayout
            };

            /// texture->RandomizeSeed();

            references.emplace_back(texture);
        }

        return references;
    }

    VkImageView EvoVulkan::Complexes::FrameBuffer::GetAttachment(uint32_t id) const  {
        if (id >= m_countColorAttach) {
            VK_ERROR("Framebuffer::GetAttachment() : going beyond the array boundaries!");
            return VK_NULL_HANDLE;
        }
        return m_attachments[id].m_view;
    }

    void EvoVulkan::Complexes::FrameBuffer::BeginCmd() {
        vkBeginCommandBuffer(*m_cmdBuff, &m_cmdBufInfo);
    }

    void EvoVulkan::Complexes::FrameBuffer::End() const {
        vkCmdEndRenderPass(*m_cmdBuff);
        vkEndCommandBuffer(*m_cmdBuff);
    }

    void EvoVulkan::Complexes::FrameBuffer::SetViewportAndScissor() const {
        vkCmdSetViewport(*m_cmdBuff, 0, 1, &m_viewport);
        vkCmdSetScissor(*m_cmdBuff, 0, 1, &m_scissor);
    }

    VkRenderPassBeginInfo EvoVulkan::Complexes::FrameBuffer::BeginRenderPass(VkClearValue *clearValues, uint32_t countCls) const {
        VkRenderPassBeginInfo renderPassBeginInfo = Tools::Initializers::RenderPassBeginInfo();

        renderPassBeginInfo.renderPass               = m_renderPass.m_self;
        renderPassBeginInfo.framebuffer              = m_framebuffer;
        renderPassBeginInfo.renderArea.extent.width  = m_width;
        renderPassBeginInfo.renderArea.extent.height = m_height;
        renderPassBeginInfo.clearValueCount          = countCls;
        renderPassBeginInfo.pClearValues             = clearValues;

        return renderPassBeginInfo;
    }

    bool EvoVulkan::Complexes::FrameBuffer::IsMultisampleEnabled() const {
        return m_device->MultisampleEnabled() && m_currentSampleCount > 1;
    }

    VkSampleCountFlagBits EvoVulkan::Complexes::FrameBuffer::GetSampleCount() const noexcept {
        return IsMultisampleEnabled() ? Tools::Convert::IntToSampleCount(m_currentSampleCount) : VK_SAMPLE_COUNT_1_BIT;
    }

    void EvoVulkan::Complexes::FrameBuffer::SetSampleCount(uint8_t sampleCount) {
        if (sampleCount == 0) {
            m_sampleCount = m_device->GetMSAASamplesCount();
        }
        else {
            m_sampleCount = EVK_MIN(sampleCount, m_device->GetMSAASamplesCount());
        }
    }

    void FrameBuffer::DeInitialize() {
        if (m_multisampleTarget) {
            m_multisampleTarget->Destroy();
            m_multisampleTarget->Free();
            m_multisampleTarget = nullptr;
        }

        if (m_attachments) {
            for (uint32_t i = 0; i < m_countColorAttach; i++)
                m_attachments[i].Destroy();
            free(m_attachments);
            m_attachments = nullptr;
        }

        if (m_framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(*m_device, m_framebuffer, nullptr);
            m_framebuffer = VK_NULL_HANDLE;
        }

        if (m_colorSampler != VK_NULL_HANDLE) {
            vkDestroySampler(*m_device, m_colorSampler, nullptr);
            m_colorSampler = VK_NULL_HANDLE;
        }
    }

    void FrameBuffer::SetLayersCount(uint32_t layersCount) {
        m_arrayLayers = layersCount;
    }

    void FrameBuffer::SetDepthAspect(VkImageAspectFlags depthAspect) {
        m_depthAspect = depthAspect;
    }

    void FrameBuffer::SetDepthFormat(VkFormat depthFormat) {
        m_depthFormat = depthFormat;
    }

    bool FrameBuffer::IsDepthEnabled() const {
        return m_depthAspect != VK_IMAGE_ASPECT_NONE && m_depthFormat != VK_FORMAT_UNDEFINED;
    }

    bool EvoVulkan::Complexes::FrameBufferAttachment::Ready() const {
        return m_image.Valid() &&
               m_view   != VK_NULL_HANDLE &&
               m_device != VK_NULL_HANDLE &&
               m_format != VK_FORMAT_UNDEFINED;
    }

    void EvoVulkan::Complexes::FrameBufferAttachment::Init() {
        m_image  = Types::Image();
        m_view   = VK_NULL_HANDLE;
        m_format = VK_FORMAT_UNDEFINED;

        m_device = VK_NULL_HANDLE;
    }

    void EvoVulkan::Complexes::FrameBufferAttachment::Destroy() {
        if (m_view) {
            vkDestroyImageView(*m_device, m_view, nullptr);
            m_view = VK_NULL_HANDLE;
        }

        if (m_image.Valid()) {
            m_allocator->FreeImage(m_image);
        }

        m_device = nullptr;
        m_allocator = nullptr;
    }
}