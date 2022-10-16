//
// Created by Nikita on 13.06.2021.
//

#include <EvoVulkan/Complexes/Framebuffer.h>
#include <EvoVulkan/Types/CmdBuffer.h>
#include <EvoVulkan/Types/CmdBuffer.h>

static EvoVulkan::Complexes::FrameBufferAttachment CreateAttachment(
        EvoVulkan::Types::Device* device,
        EvoVulkan::Memory::Allocator* allocator,
        EvoVulkan::Types::CmdPool* pool,
        VkFormat format,
        VkImageUsageFlags usage,
        VkExtent2D imageSize)
{
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
            1 /** sample count */
    );

    FBOAttachment.m_image = EvoVulkan::Types::Image::Create(imageCI);

    /// ставим барьер памяти, чтобы можно было использовать в шейдерах
    {
        auto &&copyCmd = EvoVulkan::Types::CmdBuffer::BeginSingleTime(device, pool);

        EvoVulkan::Tools::TransitionImageLayout(copyCmd, FBOAttachment.m_image, VK_IMAGE_LAYOUT_UNDEFINED,
                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        copyCmd->Destroy();
        copyCmd->Free();
    }

    FBOAttachment.m_view = EvoVulkan::Tools::CreateImageView(
            *device,
            FBOAttachment.m_image,
            format,
            1,
            aspectMask
    );

    FBOAttachment.m_format = format;
    FBOAttachment.m_device = device;
    FBOAttachment.m_allocator = allocator;

    return std::move(FBOAttachment);
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

    m_device    = nullptr;
    m_swapchain = nullptr;
    m_cmdPool   = nullptr;

    delete this;
}

std::vector<VkDescriptorImageInfo> EvoVulkan::Complexes::FrameBuffer::GetImageDescriptors() const  {
    auto descriptors = std::vector<VkDescriptorImageInfo>();

    for (uint32_t i = 0; i < m_countColorAttach; ++i) {
        descriptors.push_back(Tools::Initializers::DescriptorImageInfo(
                m_colorSampler,
                m_attachments[i].m_view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    }

    return descriptors;
}

EvoVulkan::Complexes::FrameBuffer *EvoVulkan::Complexes::FrameBuffer::Create(
        EvoVulkan::Types::Device *device,
        Memory::Allocator* allocator,
        Core::DescriptorManager* manager,
        EvoVulkan::Types::Swapchain *swapchain,
        EvoVulkan::Types::CmdPool *pool,
        const std::vector<VkFormat> &colorAttachments,
        uint32_t width,
        uint32_t height,
        float scale,
        uint8_t sampleCount,
        bool depth)
{
    if (scale <= 0.f) {
        VK_ERROR("Framebuffer::Create() : scale <= zero!");
        return nullptr;
    }

    auto fbo = new FrameBuffer();
    {
        fbo->m_scale              = scale;
        fbo->m_cmdPool            = pool;
        fbo->m_device             = device;
        fbo->m_allocator          = allocator;
        fbo->m_descriptorManager  = manager;
        fbo->m_swapchain          = swapchain;
        fbo->m_countColorAttach   = colorAttachments.size();
        fbo->m_attachFormats      = colorAttachments;
        fbo->m_depthFormat        = Tools::GetDepthFormat(*device);
        fbo->m_depthEnabled       = depth;
    }

    fbo->SetSampleCount(sampleCount);

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
    Destroy();

    m_multisampleTarget = Types::MultisampleTarget::Create(
            m_device,
            m_allocator,
            m_cmdPool,
            m_swapchain,
            width, height,
            m_attachFormats,
            GetSampleCount(),
            m_depthEnabled
    );

    if (!m_multisampleTarget) {
        VK_ERROR("Framebuffer::ReCreate() : failed to create multisample target!");
        return false;
    }

    m_baseWidth  = width;
    m_baseHeight = height;

    m_width  = m_baseWidth  * m_scale;
    m_height = m_baseHeight * m_scale;

    m_viewport = Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
    m_scissor  = Tools::Initializers::Rect2D(m_width, m_height, 0, 0);

    if (!CreateAttachments() || !CreateFramebuffer() || !CreateSampler()) {
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

    if (m_depthEnabled) {
        attachments.push_back(m_multisampleTarget->GetDepth());
    }

    /// VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {};
    /// framebufferAttachmentsCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;

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
    std::vector<VkAttachmentDescription> attachmentDescs = {};
    VkAttachmentDescription attachmentDesc = {};

    /// Init attachment properties
    for (uint32_t i = 0; i < m_countColorAttach + (m_depthEnabled ? 1 : 0); ++i) {
        attachmentDesc.samples = GetSampleCount();
        attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if (i == m_countColorAttach) {
            attachmentDesc.format = m_depthFormat;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else {
            attachmentDesc.format = m_attachFormats[i];
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        attachmentDescs.push_back(attachmentDesc);
    }

    m_renderPass = Types::CreateRenderPass(
        m_device,
        m_swapchain,
        attachmentDescs,
        Tools::Convert::SampleCountToInt(GetSampleCount()),
        m_depthEnabled
    );

    if (!m_renderPass.Ready()) {
        VK_ERROR("Framebuffer::CreateRenderPass() : failed to create render pass!");
        return false;
    }

    /// clear values
    {
        m_clearValues.clear();

        for (uint32_t i = 0; i < m_countColorAttach * (IsMultisampleEnabled() ? 2 : 1); ++i) {
            m_clearValues.emplace_back(VkClearValue{.color = {{0.0, 0.0, 0.0, 1.0}}});
        }

        if (m_depthEnabled) {
            m_clearValues.emplace_back(VkClearValue{.depthStencil = {1.0, 0}});
        }

        m_countClearValues = m_clearValues.size();
    }

    return true;
}

bool EvoVulkan::Complexes::FrameBuffer::CreateAttachments()  {
    m_attachments = (FrameBufferAttachment*)malloc(sizeof(FrameBufferAttachment) * m_countColorAttach);

    if (!m_attachments) {
        VK_ERROR("Framebuffer::CreateAttachments() : failed to allocate memory!");
        return false;
    }
    else {
        for (uint32_t i = 0; i < m_countColorAttach; ++i)
            m_attachments[i].Init();
    }

    for (uint32_t i = 0; i < m_countColorAttach; ++i) {
        m_attachments[i] = CreateAttachment(
                m_device,
                m_allocator,
                m_cmdPool,
                m_attachFormats[i],
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                { m_width, m_height }
        );

        if (!m_attachments[i].Ready()) {
            VK_ERROR("Framebuffer::CreateAttachments() : failed to create color attachment!");
            return false;
        }
    }

    return true;
}

EvoVulkan::Types::Texture *EvoVulkan::Complexes::FrameBuffer::AllocateDepthTextureReference() {
    if (!m_depthEnabled) {
        return nullptr;
    }

    auto&& texture = new EvoVulkan::Types::Texture();

    texture->m_view              = m_multisampleTarget->GetDepth();
    texture->m_image             = m_multisampleTarget->GetDepthImage().Copy();
    texture->m_format            = m_depthFormat; /// TODO: why used VK_FORMAT_B8G8R8A8_UNORM?
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
    vkBeginCommandBuffer(m_cmdBuff, &m_cmdBufInfo);
}

void EvoVulkan::Complexes::FrameBuffer::End() const {
    vkCmdEndRenderPass(m_cmdBuff);
    vkEndCommandBuffer(m_cmdBuff);
}

void EvoVulkan::Complexes::FrameBuffer::SetViewportAndScissor() const {
    vkCmdSetViewport(m_cmdBuff, 0, 1, &m_viewport);
    vkCmdSetScissor(m_cmdBuff, 0, 1, &m_scissor);
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
    return m_device->MultisampleEnabled() && m_sampleCount > 1;
}

VkSampleCountFlagBits EvoVulkan::Complexes::FrameBuffer::GetSampleCount() const noexcept {
    return IsMultisampleEnabled() ? Tools::Convert::IntToSampleCount(m_sampleCount) : VK_SAMPLE_COUNT_1_BIT;
}

void EvoVulkan::Complexes::FrameBuffer::SetSampleCount(uint8_t sampleCount) {
    if (sampleCount == 0) {
        m_sampleCount = m_device->GetMSAASamplesCount();
    }
    else {
        m_sampleCount = EVK_MIN(sampleCount, m_device->GetMSAASamplesCount());
    }
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
