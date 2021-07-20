//
// Created by Nikita on 13.06.2021.
//

#include <EvoVulkan/Complexes/Framebuffer.h>

static EvoVulkan::Complexes::FrameBufferAttachment CreateAttachment(
        EvoVulkan::Types::Device* device,
        VkFormat format,
        VkImageUsageFlags usage,
        VkExtent2D imageSize)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    if (aspectMask == VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM) {
        VK_ERROR("Types::CreateAttachment() : incorrect image usage!");
        return {};
    }

    EvoVulkan::Complexes::FrameBufferAttachment FBOAttachment = {};

    FBOAttachment.m_image = EvoVulkan::Tools::CreateImage(
            device,
            imageSize.width,
            imageSize.height,
            1,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &FBOAttachment.m_mem,
            false);

    FBOAttachment.m_view = EvoVulkan::Tools::CreateImageView(
            *device,
            FBOAttachment.m_image,
            format,
            1,
            aspectMask);

    FBOAttachment.m_format = format;
    FBOAttachment.m_device = device;

    return FBOAttachment;
}

void EvoVulkan::Complexes::FrameBuffer::Free()  {
    if (m_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(*m_device, m_semaphore, nullptr);
        m_semaphore = VK_NULL_HANDLE;
    }

    if (m_cmdBuff != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(*m_device, *m_cmdPool, 1, &m_cmdBuff);
        m_cmdBuff = VK_NULL_HANDLE;
    }

    if (m_renderPass.Ready())
        Types::DestroyRenderPass(m_device, &m_renderPass);

    this->m_device    = nullptr;
    this->m_swapchain = nullptr;
    this->m_cmdPool   = nullptr;

    delete this;
}

std::vector<VkDescriptorImageInfo> EvoVulkan::Complexes::FrameBuffer::GetImageDescriptors() const  {
    auto descriptors = std::vector<VkDescriptorImageInfo>();

    for (uint32_t i = 0; i < m_countColorAttach; i++)
        descriptors.push_back(Tools::Initializers::DescriptorImageInfo(
                m_colorSampler,
                m_attachments[i].m_view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    return descriptors;
}

EvoVulkan::Complexes::FrameBuffer *EvoVulkan::Complexes::FrameBuffer::Create(
        EvoVulkan::Types::Device *device,
        EvoVulkan::Types::Swapchain *swapchain,
        EvoVulkan::Types::CmdPool *pool,
        const std::vector<VkFormat> &colorAttachments,
        uint32_t width,
        uint32_t height,
        float scale)
{
    if (scale <= 0.f) {
        VK_ERROR("Framebuffer::Create() : scale <= zero!");
        return nullptr;
    }

    auto fbo = new FrameBuffer();
    {
        fbo->m_scale            = scale;
        fbo->m_cmdPool          = pool;
        fbo->m_device           = device;
        fbo->m_swapchain        = swapchain;
        fbo->m_countColorAttach = colorAttachments.size();
        fbo->m_attachFormats    = colorAttachments;
        fbo->m_depthFormat      = Tools::GetDepthFormat(*device);
    }

    auto semaphoreCI  = Tools::Initializers::SemaphoreCreateInfo();
    if (vkCreateSemaphore(*device, &semaphoreCI, nullptr, &fbo->m_semaphore) != VK_SUCCESS) {
        VK_ERROR("Framebuffer::Create() : failed to create vulkan semaphore!");
        return nullptr;
    }

    fbo->m_cmdBuff    = Types::CmdBuffer::CreateSimple(device, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    fbo->m_cmdBufInfo = Tools::Initializers::CommandBufferBeginInfo();

    if (!fbo->CreateRenderPass()) {
        VK_ERROR("Framebuffer::Create() : failed to create render pass!");
        return nullptr;
    }

    if (!fbo->ReCreate(width, height)) {
        VK_ERROR("Framebuffer::Create() : failed to re-create framebuffer!");
        return nullptr;
    }

    return fbo;
}

bool EvoVulkan::Complexes::FrameBuffer::ReCreate(uint32_t width, uint32_t height)  {
    this->Destroy();

    this->m_multisampleTarget = Types::MultisampleTarget::Create(
            m_device,
            m_swapchain,
            width, height,
            m_attachFormats,
            m_device->MultisampleEnabled());
    if (!m_multisampleTarget) {
        VK_ERROR("Framebuffer::ReCreate() : failed to create multisample target!");
        return false;
    }

    this->m_baseWidth  = width;
    this->m_baseHeight = height;

    this->m_width    = m_baseWidth  * m_scale;
    this->m_height   = m_baseHeight * m_scale;

    this->m_viewport = Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
    this->m_scissor  = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);

    if (!this->CreateAttachments() ||
        !this->CreateFramebuffer() ||
        !this->CreateSampler()
            ) {
        VK_ERROR("Framebuffer::ReCreate() : failed to re-create frame buffer!");
        return false;
    }

    return true;
}

void EvoVulkan::Complexes::FrameBuffer::Destroy()  {
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

    for (uint32_t i = 0; i < m_countColorAttach; i++) {
        if (m_device->MultisampleEnabled())
            attachments.push_back(m_multisampleTarget->GetResolve(i));
        attachments.push_back(m_attachments[i].m_view);
        /*
         *  0 - resolve
         *  1 - color
         *  2 - resolve
         *  3 - color
         *  ...
         *  n - depth
         */
    }

    attachments.push_back(m_multisampleTarget->GetDepth());

    VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {};
    framebufferAttachmentsCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;

    VkFramebufferCreateInfo FBO_CI = {};
    //FBO_CI.flags                   = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
    FBO_CI.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    //FBO_CI.pNext                   = &framebufferAttachmentsCreateInfo;
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
    std::vector<VkAttachmentDescription> attachmentDescs = {};
    VkAttachmentDescription attachmentDesc = {};

    // Init attachment properties
    for (uint32_t i = 0; i < m_countColorAttach + (m_depthEnabled ? 1 : 0); ++i) {
        attachmentDesc.samples = m_device->GetMSAASamples();
        attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if (i == m_countColorAttach) {
            attachmentDesc.format = m_depthFormat;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        } else {
            attachmentDesc.format = m_attachFormats[i];
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        attachmentDescs.push_back(attachmentDesc);
    }

    this->m_renderPass = Types::CreateRenderPass(m_device, m_swapchain, attachmentDescs,
                                                 m_device->MultisampleEnabled(), m_depthEnabled);
    if (!m_renderPass.Ready()) {
        VK_ERROR("Framebuffer::CreateRenderPass() : failed to create render pass!");
        return false;
    }

    //clear values
    {
        this->m_clearValues.clear();
        for (uint32_t i = 0; i < m_countColorAttach * (m_device->MultisampleEnabled() ? 2 : 1); i++)
            m_clearValues.emplace_back(VkClearValue{ .color = {{ 0.0, 0.0, 0.0, 0.0 }} });
        m_clearValues.emplace_back(VkClearValue{ .depthStencil = { 0.0, 0 } }); // TODO: maybe check depth enabled?
        this->m_countClearValues = m_clearValues.size();
    }

    return true;
}

bool EvoVulkan::Complexes::FrameBuffer::CreateAttachments()  {
    this->m_attachments = (FrameBufferAttachment*)malloc(sizeof(FrameBufferAttachment) * m_countColorAttach);
    if (!m_attachments) {
        VK_ERROR("Framebuffer::CreateAttachments() : failed to allocate memory!");
        return false;
    } else
        for (uint32_t i = 0; i < m_countColorAttach; i++)
            m_attachments[i].Init();

    for (uint32_t i = 0; i < m_countColorAttach; i++) {
        m_attachments[i] = CreateAttachment(
                m_device,
                m_attachFormats[i],
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                { m_width, m_height });

        auto copyCmd = Types::CmdBuffer::BeginSingleTime(m_device, m_cmdPool);
        Tools::TransitionImageLayout(copyCmd, m_attachments[i].m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
        copyCmd->Destroy();
        copyCmd->Free();

        if (!m_attachments[i].Ready()) {
            VK_ERROR("Framebuffer::CreateAttachments() : failed to create attachment!");
            return false;
        }
    }

    return true;
}

std::vector<EvoVulkan::Types::Texture*> EvoVulkan::Complexes::FrameBuffer::AllocateColorTextureReferences() {
    auto references = std::vector<EvoVulkan::Types::Texture*>();

    for (uint32_t i = 0; i < m_countColorAttach; i++) {
        auto* texture = new EvoVulkan::Types::Texture();

        texture->m_deviceMemory   = m_attachments[i].m_mem;
        texture->m_view           = m_attachments[i].m_view;
        texture->m_image          = m_attachments[i].m_image;
        texture->m_format         = m_attachments[i].m_format;
        texture->m_sampler        = m_colorSampler;
        texture->m_imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texture->m_device         = m_device;
        texture->m_width          = m_width;
        texture->m_height         = m_height;
        texture->m_canBeDestroyed = false;
        texture->m_mipLevels      = 1;

        //! make a texture descriptor
        texture->m_descriptor = {
                texture->m_sampler,
                texture->m_view,
                texture->m_imageLayout
        };

        texture->RandomizeSeed();

        references.emplace_back(texture);
    }

    return references;
}
