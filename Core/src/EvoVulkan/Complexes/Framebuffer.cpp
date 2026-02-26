//
// Created by Nikita on 13.06.2021.
//

#include <EvoVulkan/Complexes/Framebuffer.h>
#include <EvoVulkan/Complexes/FrameBufferLayer.h>
#include <EvoVulkan/Complexes/FrameBufferAttachment.h>
#include <EvoVulkan/Types/CmdBuffer.h>

namespace EvoVulkan::Complexes {
    FrameBuffer::~FrameBuffer() {
        DeInitialize();

        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(*m_device, m_semaphore, nullptr);
            m_semaphore = VK_NULL_HANDLE;
        }

        for (auto&& pCmdBuffer : m_cmdBuffers) {
            delete pCmdBuffer;
        }
        m_cmdBuffers.clear();

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
        std::vector<Types::CmdPool*> framePools,
        FrameBufferFeatures features,
        const std::vector<VkFormat>& colorAttachments,
        uint32_t width, uint32_t height,
        uint32_t layersCount,
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

        if (layersCount <= 0 || layersCount > 256) {
            VK_ERROR("Framebuffer::Create() : invalid layers!");
            return nullptr;
        }

        if (arrayLayers <= 0 || arrayLayers > 256) {
            VK_ERROR("Framebuffer::Create() : invalid array layers!");
            return nullptr;
        }

        if (colorAttachments.size() > 0 && layersCount > 1) {
            VK_ERROR("Framebuffer::Create() : layers > 1 is not supported yet for multiple color attachments!");
            return nullptr;
        }

        if (colorAttachments.size() > 0 && arrayLayers > 1) {
            VK_ERROR("Framebuffer::Create() : array layers > 1 is not supported yet for multiple color attachments!");
            return nullptr;
        }

        if (!Tools::IsFormatInRange(depthFormat)) {
            VK_HALT("FrameBuffer::Create() : format is in not a range!");
            return nullptr;
        }

        auto&& pFBO = new FrameBuffer();
        {
            pFBO->m_layersCount        = layersCount;
            pFBO->m_arrayLayersCount   = arrayLayers;
            pFBO->m_scale              = scale;
            pFBO->m_cmdPool            = pool;
            pFBO->m_cmdPools           = std::move(framePools);
            pFBO->m_device             = device;
            pFBO->m_allocator          = allocator;
            pFBO->m_descriptorManager  = manager;
            pFBO->m_swapchain          = swapchain;
            pFBO->m_attachFormats      = colorAttachments;
            pFBO->m_depthFormat        = depthFormat;
            pFBO->m_depthAspect        = depthAspect;
            pFBO->m_features           = features;
        }

        pFBO->SetSampleCount(samplesCount);

        auto&& semaphoreCI = Tools::Initializers::SemaphoreCreateInfo();
        if (vkCreateSemaphore(*device, &semaphoreCI, nullptr, &pFBO->m_semaphore) != VK_SUCCESS) {
            VK_ERROR("Framebuffer::Create() : failed to create vulkan semaphore!");
            return nullptr;
        }

        const uint8_t maxFrames = swapchain->GetCountImages();
        for (uint32_t i = 0; i < maxFrames; ++i) {
            pFBO->m_cmdBuffers.emplace_back(Types::CmdBuffer::Create(device, pFBO->m_cmdPools[i], VK_COMMAND_BUFFER_LEVEL_PRIMARY));
        }

        if (!pFBO->ReCreate(width, height)) {
            VK_ERROR("Framebuffer::Create() : failed to re-create framebuffer!");
            return nullptr;
        }

        return pFBO;
    }

    bool FrameBuffer::ReCreate(uint32_t width, uint32_t height)  {
        DeInitialize();

        m_baseWidth  = width;
        m_baseHeight = height;

        m_width  = m_baseWidth  * m_scale;
        m_height = m_baseHeight * m_scale;

        if (m_layersCount > 32 || m_layersCount == 0) {
            VK_ERROR("Framebuffer::ReCreate() : invalid layers count!");
            return false;
        }

        if (m_width <= 0 || m_height <= 0) {
            VK_ERROR("Framebuffer::ReCreate() : invalid sizes!");
            return false;
        }

        if (!CreateRenderPass()) {
            VK_ERROR("Framebuffer::ReCreate() : failed to create render pass!");
            return false;
        }

        m_viewport = Tools::Initializers::Viewport((float_t)m_width, (float_t)m_height, 0.0f, 1.0f);
        m_scissor = Tools::Initializers::Rect2D(0, 0, m_width, m_height);

        if (!CreateAttachments()) {
            VK_ERROR("Framebuffer::ReCreate() : failed to create render pass!");
            return false;
        }

        if (!CreateFramebuffer() || !CreateSampler()) {
            VK_ERROR("Framebuffer::ReCreate() : failed to re-create frame buffer!");
            return false;
        }

        return true;
    }

    bool FrameBuffer::CreateSampler()  {
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

    bool EvoVulkan::Complexes::FrameBuffer::CreateFramebuffer() {
        VK_GRAPH("Framebuffer::CreateFramebuffer() : create vulkan framebuffer...");

        for (uint32_t layerIndex = 0; layerIndex < m_layersCount; ++layerIndex) {
            std::vector<VkImageView> attachments;

            /**
             *  0 - resolve
             *  1 - color
             *  2 - resolve
             *  3 - color
             *  ...
             *  n - depth
            */

            for (uint32_t colorAttachIndex = 0; colorAttachIndex < m_attachFormats.size(); ++colorAttachIndex) {
                if (IsMultisampleEnabled()) {
                    attachments.push_back(m_layers[layerIndex]->GetColorAttachments()[colorAttachIndex]->GetView());
                    attachments.push_back(m_layers[layerIndex]->GetResolveAttachments()[colorAttachIndex]->GetView());
                }
                else {
                    attachments.push_back(m_layers[layerIndex]->GetColorAttachments()[colorAttachIndex]->GetView());
                }
            }

            if (IsDepthEnabled()) {
                attachments.push_back(m_layers[layerIndex]->GetDepthAttachment()->GetView());
                // Add depth resolve attachment if multisampling is enabled and depthShaderRead is true
                if (IsMultisampleEnabled() && m_features.depthShaderRead && m_layers[layerIndex]->GetDepthResolveAttachment()) {
                    attachments.push_back(m_layers[layerIndex]->GetDepthResolveAttachment()->GetView());
                }
            }

            if (m_renderPass.m_countAttachments != attachments.size()) {
                VK_ERROR("Framebuffer::CreateFramebuffer() : incompatible attachments!"
                         "\n\tRender pass: " + std::to_string(m_renderPass.m_countAttachments) +
                         "\n\tFramebuffer: " + std::to_string(attachments.size())
                );
                return false;
            }

            VkFramebufferCreateInfo FBO_CI = { };
            FBO_CI.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FBO_CI.renderPass              = m_renderPass.m_self;
            FBO_CI.pAttachments            = attachments.data();
            FBO_CI.attachmentCount         = static_cast<uint32_t>(attachments.size());
            FBO_CI.width                   = m_width;
            FBO_CI.height                  = m_height;
            FBO_CI.layers                  = m_arrayLayersCount;

            VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
            const bool isSuccess = vkCreateFramebuffer(*m_device, &FBO_CI, nullptr, &vkFramebuffer) == VK_SUCCESS;
            m_layers[layerIndex]->SetFrameBuffer(vkFramebuffer);

            if (!isSuccess || vkFramebuffer == VK_NULL_HANDLE) {
                VK_ERROR("Framebuffer::CreateFramebuffer() : failed to create vulkan framebuffer!");
                return false;
            }
        }

        return true;
    }

    bool FrameBuffer::CreateRenderPass() {
        m_currentSampleCount = m_sampleCount;

        if (m_renderPass.IsReady()) {
            if (m_dirtyRenderPass) {
                Types::DestroyRenderPass(m_device, &m_renderPass);
            }
            else {
                return true;
            }
        }

        m_dirtyRenderPass = false;

        std::vector<VkAttachmentDescription2> attachmentDescriptions;
        VkAttachmentDescription2 attachmentDesc = { };

        /// Init attachment properties
        // Calculate total attachments: color + resolve + depth + depth resolve (if needed)
        uint32_t totalAttachments = m_attachFormats.size() * (IsMultisampleEnabled() ? 2 : 1) + 
                                    (IsDepthEnabled() ? 1 : 0) + 
                                    (IsDepthEnabled() && IsMultisampleEnabled() && m_features.depthShaderRead ? 1 : 0);
        
        for (uint32_t i = 0; i < totalAttachments; ++i) {
            attachmentDesc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            attachmentDesc.pNext = nullptr;
            attachmentDesc.samples = GetSampleCount();
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; /// may be VK_ATTACHMENT_LOAD_OP_CLEAR?
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // Determine attachment type
            uint32_t colorResolveCount = m_attachFormats.size() * (IsMultisampleEnabled() ? 2 : 1);
            uint32_t depthIndex = colorResolveCount;
            uint32_t depthResolveIndex = depthIndex + (IsDepthEnabled() ? 1 : 0);

            if (i >= depthResolveIndex && IsDepthEnabled() && IsMultisampleEnabled() && m_features.depthShaderRead) {
                // Depth resolve attachment
                attachmentDesc.format = m_depthFormat;
                attachmentDesc.samples = Tools::Convert::IntToSampleCount(1);
                attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachmentDesc.finalLayout = Tools::FindDepthFormatLayout(m_depthAspect, true, false);
            }
            else if (i == depthIndex && IsDepthEnabled()) {
                // Depth attachment
                attachmentDesc.format = m_depthFormat;
                if (m_features.depthLoad) {
                    attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                }
                if (m_features.depthShaderRead) {
                    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachmentDesc.finalLayout = Tools::FindDepthFormatLayout(m_depthAspect, true, false);
                }
                else {
                    //attachmentDesc.initialLayout = Tools::FindDepthFormatLayout(m_depthAspect, false, false);
                    attachmentDesc.initialLayout = Tools::FindDepthFormatLayout(m_depthAspect, m_features.depthShaderRead, false);
                    attachmentDesc.finalLayout = attachmentDesc.initialLayout;
                }
            }
            else {
                // Color attachments (multisampled) and resolve attachments
                uint32_t colorIndex = i;
                if (IsMultisampleEnabled() && (i % 2 == 1)) {
                    // This is a resolve attachment
                    colorIndex = (i - 1) / 2;
                    attachmentDesc.samples = Tools::Convert::IntToSampleCount(1);
                    attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
                else if (IsMultisampleEnabled()) {
                    // This is a multisampled color attachment
                    colorIndex = i / 2;
                }
                
                if (colorIndex < m_attachFormats.size()) {
                    attachmentDesc.format = m_attachFormats[colorIndex];
                    if (!IsMultisampleEnabled() || (i % 2 == 0)) {
                        // Only set these for non-resolve attachments
                        if (m_features.colorLoad) {
                            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                        }

                        if (m_features.colorShaderRead) {
                            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        }
                        else {
                            attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                            attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        }
                    }
                }
            }

            attachmentDescriptions.push_back(attachmentDesc);
        }

        std::vector<VkAttachmentReference2> inputAttachments;

        m_renderPass = Types::CreateRenderPass(
            m_device,
            m_swapchain,
            attachmentDescriptions,
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

            for (uint32_t i = 0; i < m_attachFormats.size() * (IsMultisampleEnabled() ? 2 : 1); ++i) {
                m_clearValues.emplace_back(VkClearValue{.color = {{0.0, 0.0, 0.0, 1.0}}});
            }

            if (IsDepthEnabled()) {
                m_clearValues.emplace_back(VkClearValue{.depthStencil = {1.0, 0}});
            }

            m_countClearValues = m_clearValues.size();
        }

        return true;
    }

    bool FrameBuffer::CreateAttachments() {
        if (IsDepthEnabled() && m_layersCount > 1) {
            m_depthAttachment = FrameBufferAttachment::CreateDepthAttachment(
                this,
                nullptr /** image array */,
                GetDepthFormat(),
                GetDepthAspect(),
                m_layersCount,
                0 /** layer index */
            );

            if (!m_depthAttachment || !m_depthAttachment->Ready()) {
                VK_ERROR("FrameBuffer::CreateAttachments() : failed to create depth attachment!");
                return false;
            }
        }

        for (uint32_t layerIndex = 0; layerIndex < m_layersCount; ++layerIndex) {
            auto&& pLayer = std::make_unique<FrameBufferLayer>(this, layerIndex, m_depthAttachment.get(), m_layersCount);

            if (!pLayer->Initialize()) {
                VK_ERROR("Framebuffer::ReCreate() : failed to initialize layer!");
                return false;
            }

            m_layers.emplace_back(std::move(pLayer));
        }

        return true;
    }

    EvoVulkan::Types::Texture *EvoVulkan::Complexes::FrameBuffer::AllocateDepthTextureReference(int32_t index) {
        if (!IsDepthEnabled()) {
            return nullptr;
        }

        auto&& texture = new EvoVulkan::Types::Texture();

        if (index >= 0) {
            if (index >= m_layers.size()) {
                VK_ASSERT(false);
                return nullptr;
            }
            // Use resolve attachment if multisampling is enabled and depthShaderRead is true
            // This is needed because multisampled depth images cannot be read directly in shaders with sampler2DArray
            if (IsMultisampleEnabled() && m_features.depthShaderRead && m_layers.at(index)->GetDepthResolveAttachment()) {
                texture->m_view = m_layers.at(index)->GetDepthResolveAttachment()->GetView();
                texture->m_image = m_layers.at(index)->GetDepthResolveAttachment()->GetImage().Copy();
            }
            else {
                texture->m_view = m_layers.at(index)->GetDepthAttachment()->GetView();
                texture->m_image = m_layers.at(index)->GetDepthAttachment()->GetImage().Copy();
            }
        }
        else if (m_layersCount > 1) {
            texture->m_view = m_depthAttachment->GetView();
            texture->m_image = m_depthAttachment->GetImage().Copy();
        }
        else if (m_layersCount > 0) {
            // Use resolve attachment if multisampling is enabled and depthShaderRead is true
            if (IsMultisampleEnabled() && m_features.depthShaderRead && m_layers.at(0)->GetDepthResolveAttachment()) {
                texture->m_view = m_layers.at(0)->GetDepthResolveAttachment()->GetView();
                texture->m_image = m_layers.at(0)->GetDepthResolveAttachment()->GetImage().Copy();
            }
            else {
                texture->m_view = m_layers.at(0)->GetDepthAttachment()->GetView();
                texture->m_image = m_layers.at(0)->GetDepthAttachment()->GetImage().Copy();
            }
        }

        texture->m_canBeDestroyed              = false;
        texture->m_sampler                     = m_colorSampler;
        texture->m_loadInfo.format             = m_depthFormat;
        texture->m_loadInfo.pDescriptorManager = m_descriptorManager;
        texture->m_loadInfo.pDevice            = m_device;
        texture->m_loadInfo.pPool              = m_cmdPool;
        texture->m_loadInfo.pAllocator         = m_allocator;
        texture->m_loadInfo.width              = m_width;
        texture->m_loadInfo.height             = m_height;
        texture->m_loadInfo.mipLevels          = 1;

        /// make a texture descriptor
        texture->m_descriptor = {
                texture->m_sampler,
                texture->m_view,
                texture->m_image.GetLayout()
        };

        return texture;
    }

    std::vector<EvoVulkan::Types::Texture*> EvoVulkan::Complexes::FrameBuffer::AllocateResolveTextureReferences() {
        if (!IsMultisampleEnabled()) {
            return AllocateColorTextureReferences();
        }
        auto&& references = std::vector<EvoVulkan::Types::Texture*>();
        for (uint32_t layerIndex = 0; layerIndex < m_layersCount; ++layerIndex) {
            for (uint32_t attachmentIndex = 0; attachmentIndex < m_attachFormats.size(); ++attachmentIndex) {
                auto&& pTexture = new EvoVulkan::Types::Texture();

                pTexture->m_canBeDestroyed              = false;
                pTexture->m_sampler                     = m_colorSampler;
                pTexture->m_view                        = m_layers[layerIndex]->GetResolveAttachments()[attachmentIndex]->GetView();
                pTexture->m_image                       = m_layers[layerIndex]->GetResolveAttachments()[attachmentIndex]->GetImage().Copy();
                pTexture->m_loadInfo.format             = m_layers[layerIndex]->GetResolveAttachments()[attachmentIndex]->GetFormat();
                pTexture->m_loadInfo.pDescriptorManager = m_descriptorManager;
                pTexture->m_loadInfo.pDevice            = m_device;
                pTexture->m_loadInfo.pPool              = m_cmdPool;
                pTexture->m_loadInfo.pAllocator         = m_allocator;
                pTexture->m_loadInfo.width              = m_width;
                pTexture->m_loadInfo.height             = m_height;
                pTexture->m_loadInfo.mipLevels          = 1;

                //! make a texture descriptor
                pTexture->m_descriptor = {
                    pTexture->m_sampler,
                    pTexture->m_view,
                    pTexture->m_image.GetLayout()
                };

                references.emplace_back(pTexture);
            }
        }
        return references;
    }

    std::vector<EvoVulkan::Types::Texture*> EvoVulkan::Complexes::FrameBuffer::AllocateColorTextureReferences() {
        auto&& references = std::vector<EvoVulkan::Types::Texture*>();

        for (uint32_t layerIndex = 0; layerIndex < m_layersCount; ++layerIndex) {
            for (uint32_t attachmentIndex = 0; attachmentIndex < m_attachFormats.size(); ++attachmentIndex) {
                auto&& pTexture = new EvoVulkan::Types::Texture();

                pTexture->m_canBeDestroyed              = false;
                pTexture->m_sampler                     = m_colorSampler;
                pTexture->m_view                        = m_layers[layerIndex]->GetColorAttachments()[attachmentIndex]->GetView();
                pTexture->m_image                       = m_layers[layerIndex]->GetColorAttachments()[attachmentIndex]->GetImage().Copy();
                pTexture->m_loadInfo.format             = m_layers[layerIndex]->GetColorAttachments()[attachmentIndex]->GetFormat();
                pTexture->m_loadInfo.pDescriptorManager = m_descriptorManager;
                pTexture->m_loadInfo.pDevice            = m_device;
                pTexture->m_loadInfo.pPool              = m_cmdPool;
                pTexture->m_loadInfo.pAllocator         = m_allocator;
                pTexture->m_loadInfo.width              = m_width;
                pTexture->m_loadInfo.height             = m_height;
                pTexture->m_loadInfo.mipLevels          = 1;

                //! make a texture descriptor
                pTexture->m_descriptor = {
                    pTexture->m_sampler,
                    pTexture->m_view,
                    pTexture->m_image.GetLayout()
                };

                references.emplace_back(pTexture);
            }
        }

        return references;
    }

    VkImageView EvoVulkan::Complexes::FrameBuffer::GetAttachment(uint32_t layer, uint32_t id) const {
        if (m_layersCount <= layer) {
            VK_ERROR("Framebuffer::GetAttachment() : out of range layer!");
            return VK_NULL_HANDLE;
        }

        if (id >= m_attachFormats.size()) {
            VK_ERROR("Framebuffer::GetAttachment() : going beyond the array boundaries!");
            return VK_NULL_HANDLE;
        }

        return m_layers[layer]->GetColorAttachments().at(id)->GetView();
    }

    //void EvoVulkan::Complexes::FrameBuffer::BeginCmd() {
    //    vkBeginCommandBuffer(*m_cmdBuff, &m_cmdBufInfo);
    //}

    //void EvoVulkan::Complexes::FrameBuffer::End() const {
    //    vkCmdEndRenderPass(*m_cmdBuff);
    //    vkEndCommandBuffer(*m_cmdBuff);
    //}

    //void EvoVulkan::Complexes::FrameBuffer::SetViewportAndScissor() const {
        //vkCmdSetViewport(*m_cmdBuff, 0, 1, &m_viewport);
        //vkCmdSetScissor(*m_cmdBuff, 0, 1, &m_scissor);
    //}

    /*VkRenderPassBeginInfo EvoVulkan::Complexes::FrameBuffer::BeginRenderPass(VkClearValue *clearValues, uint32_t countCls, uint32_t layer) const {
        VkRenderPassBeginInfo renderPassBeginInfo = Tools::Initializers::RenderPassBeginInfo();

        renderPassBeginInfo.renderPass               = m_renderPass.m_self;
        renderPassBeginInfo.framebuffer              = m_layers[layer]->GetFramebuffer();
        renderPassBeginInfo.renderArea.extent.width  = m_width;
        renderPassBeginInfo.renderArea.extent.height = m_height;
        renderPassBeginInfo.clearValueCount          = countCls;
        renderPassBeginInfo.pClearValues             = clearValues;

        return renderPassBeginInfo;
    }*/

    bool EvoVulkan::Complexes::FrameBuffer::IsMultisampleEnabled() const {
        if (m_currentSampleCount != m_sampleCount) {
            VK_HALT("FrameBuffer::IsMultisampleEnabled() : current sample count is not equal to the specified one!");
        }
        return m_device->IsMultiSamplingEnabled() && m_currentSampleCount > 1;
    }

    VkSampleCountFlagBits EvoVulkan::Complexes::FrameBuffer::GetSampleCount() const noexcept {
        if (m_currentSampleCount != m_sampleCount) {
            VK_HALT("FrameBuffer::GetSampleCount() : current sample count is not equal to the specified one!");
        }
        return IsMultisampleEnabled() ? Tools::Convert::IntToSampleCount(m_currentSampleCount) : VK_SAMPLE_COUNT_1_BIT;
    }

    void EvoVulkan::Complexes::FrameBuffer::SetSampleCount(uint8_t sampleCount) {
        uint8_t oldSampleCount = m_sampleCount;

        if (sampleCount == 0) {
            m_sampleCount = m_device->GetMSAASamplesCount();
        }
        else {
            m_sampleCount = EVK_MIN(sampleCount, m_device->GetMSAASamplesCount());
        }

        m_dirtyRenderPass |= (oldSampleCount != m_sampleCount);
    }

    void FrameBuffer::DeInitialize() {
        m_layers.clear();
        m_depthAttachment.reset();

        if (m_colorSampler != VK_NULL_HANDLE) {
            vkDestroySampler(*m_device, m_colorSampler, nullptr);
            m_colorSampler = VK_NULL_HANDLE;
        }
    }

    void FrameBuffer::SetLayersCount(uint32_t layersCount) {
        m_dirtyRenderPass |= (m_layersCount != layersCount);
        m_layersCount = layersCount;
    }

    void FrameBuffer::SetArrayLayersCount(uint32_t arrayLayersCount) {
        m_dirtyRenderPass |= (m_arrayLayersCount != arrayLayersCount);
        m_arrayLayersCount = arrayLayersCount;
    }

    void FrameBuffer::SetDepthAspect(VkImageAspectFlags depthAspect) {
        m_dirtyRenderPass |= (m_depthAspect != depthAspect);
        m_depthAspect = depthAspect;
    }

    void FrameBuffer::SetDepthFormat(VkFormat depthFormat) {
        if (Tools::IsFormatInRange(depthFormat)) {
            VK_HALT("FrameBuffer::SetDepthFormat() : format is in not a range!");
            return;
        }
        m_dirtyRenderPass |= (m_depthFormat != depthFormat);
        m_depthFormat = depthFormat;
    }

    bool FrameBuffer::IsDepthEnabled() const {
        return m_depthAspect != EvoVulkan::Tools::Initializers::EVK_IMAGE_ASPECT_NONE && m_depthFormat != VK_FORMAT_UNDEFINED;
    }

    void FrameBuffer::ClearSignalSemaphores() {
        m_signalSemaphores.clear();
        m_signalSemaphores.emplace_back(GetSemaphore());
    }

    std::vector<Types::Texture*> FrameBuffer::AllocateDepthTextureReferences() {
        std::vector<Types::Texture*> attachments;

        if (m_depthAspect != VK_IMAGE_ASPECT_DEPTH_BIT) {
            return attachments;
        }

        for (uint32_t layerIndex = 0; layerIndex < m_layersCount; ++layerIndex) {
            if (auto&& pTexture = AllocateDepthTextureReference(layerIndex)) {
                attachments.emplace_back(pTexture);
            }
        }
        return attachments;
    }

    void FrameBuffer::SetFeatures(const FrameBufferFeatures& features) {
        m_dirtyRenderPass |= (m_features != features);
        m_features = features;
    }

    VkCommandBuffer FrameBuffer::GetCommandBuffer(uint32_t frame) const {
        if (!m_cmdBuffers.size()) {
            VK_HALT("Framebuffer::GetCommandBuffer() : no command buffers!");
            return VK_NULL_HANDLE;
        }

        if (frame >= m_cmdBuffers.size()) {
            VK_HALT("Framebuffer::GetCommandBuffer() : out of range frame index!");
            return VK_NULL_HANDLE;
        }
        return *m_cmdBuffers[frame];
    }
}