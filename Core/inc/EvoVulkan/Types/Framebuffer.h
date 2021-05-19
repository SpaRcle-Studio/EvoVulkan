//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_FRAMEBUFFER_H
#define EVOVULKAN_FRAMEBUFFER_H

#include <vulkan/vulkan.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanTools.h>

namespace EvoVulkan::Types {
    struct FrameBufferAttachment {
        VkImage        m_image;
        VkDeviceMemory m_mem;
        VkImageView    m_view;
        VkFormat       m_format;

        VkDevice       m_device;

        [[nodiscard]] bool Ready() const {
            return m_image  != VK_NULL_HANDLE &&
                   m_mem    != VK_NULL_HANDLE &&
                   m_view   != VK_NULL_HANDLE &&
                   m_device != VK_NULL_HANDLE &&
                   m_format != VK_FORMAT_UNDEFINED;
        }

        void Init() {
            m_image  = VK_NULL_HANDLE;
            m_mem    = VK_NULL_HANDLE;
            m_view   = VK_NULL_HANDLE;
            m_format = VK_FORMAT_UNDEFINED;

            m_device = VK_NULL_HANDLE;
        }

        void Destroy() {
            if (m_view) {
                vkDestroyImageView(m_device, m_view, nullptr);
                m_view = VK_NULL_HANDLE;
            }

            if (m_image) {
                vkDestroyImage(m_device, m_image, nullptr);
                m_image = VK_NULL_HANDLE;
            }

            if (m_mem) {
                vkFreeMemory(m_device, m_mem, nullptr);
                m_mem = VK_NULL_HANDLE;
            }

            m_device = VK_NULL_HANDLE;
        }
    };

    static FrameBufferAttachment CreateAttachment(
            const Types::Device* device,
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

        FrameBufferAttachment FBOAttachment = {};

        FBOAttachment.m_image = Tools::CreateImage(
                                    device,
                                    imageSize.width,
                                    imageSize.height,
                                    1,
                                    format,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    usage,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    &FBOAttachment.m_mem);

        FBOAttachment.m_view = Tools::CreateImageView(
                *device,
                FBOAttachment.m_image,
                format,
                1,
                aspectMask);

        FBOAttachment.m_format = format;
        FBOAttachment.m_device = *device;

        return FBOAttachment;
    }

    //!=================================================================================================================

    class FrameBuffer {
        uint32_t                m_width         = 0,
                                m_height        = 0,
                                m_countAttach   = 0;

        VkFramebuffer           m_framebuffer   = VK_NULL_HANDLE;
        VkRenderPass            m_renderPass    = VK_NULL_HANDLE;

        VkSampler               m_colorSampler  = VK_NULL_HANDLE;

        std::vector<VkFormat>   m_attachFormats = {};
        VkFormat                m_depthFormat   = VK_FORMAT_UNDEFINED;

        FrameBufferAttachment*  m_attachments   = nullptr;
        FrameBufferAttachment   m_depth         = {};

        const Types::Device*    m_device        = nullptr;
        const Types::Swapchain* m_swapchain     = nullptr;
    private:
        bool CreateAttachments() {
            this->m_attachments = (FrameBufferAttachment*)malloc(sizeof(FrameBufferAttachment) * m_countAttach);
            if (!m_attachments) {
                VK_ERROR("Framebuffer::CreateAttachments() : failed to allocate memory!");
                return false;
            } else
                for (uint32_t i = 0; i < m_countAttach; i++)
                    m_attachments[i].Init();

           for (uint32_t i = 0; i < m_countAttach; i++) {
               m_attachments[i] = CreateAttachment(
                       m_device,
                       m_attachFormats[i],
                       VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                       { m_width, m_height });

               if (!m_attachments[i].Ready()) {
                   VK_ERROR("Framebuffer::CreateAttachments() : failed to create attachment!");
                   return false;
               }
           }

           m_depth = CreateAttachment(
                   m_device,
                   m_depthFormat,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                   { m_width, m_height });
           if (!m_depth.Ready()) {
               VK_ERROR("Framebuffer::CreateAttachments() : failed to create depth attachment!");
               return false;
           }

            return true;
        }
        bool CreateRenderPass() {
            std::vector<VkAttachmentDescription> attachmentDescs = {};
            // Init attachment properties
            for (uint32_t i = 0; i < m_countAttach + 1; ++i) {
                VkAttachmentDescription attachmentDesc = {};

                attachmentDesc.samples        = VK_SAMPLE_COUNT_1_BIT; // TODO: 1 sample bit! Need get from device.
                attachmentDesc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachmentDesc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                attachmentDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

                if (i == m_countAttach) {
                    attachmentDesc.format        = m_depthFormat;
                    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachmentDesc.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else {
                    attachmentDesc.format        = m_attachFormats[i];
                    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachmentDesc.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                attachmentDescs.push_back(attachmentDesc);
            }

            this->m_renderPass = Tools::CreateRenderPass(m_device, m_swapchain, attachmentDescs);
            if (m_renderPass == VK_NULL_HANDLE) {
                VK_ERROR("Framebuffer::CreateRenderPass() : failed to create render pass!");
                return false;
            }

            return true;
        }
        bool CreateFramebuffer() {
            std::vector<VkImageView> attachments = {};
            for (uint32_t i = 0; i < m_countAttach; i++)
                attachments.push_back(m_attachments[i].m_view);
            attachments.push_back(m_depth.m_view);

            VkFramebufferCreateInfo FBO_CI = {};
            FBO_CI.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FBO_CI.pNext                   = NULL;
            FBO_CI.renderPass              = m_renderPass;
            FBO_CI.pAttachments            = attachments.data();
            FBO_CI.attachmentCount         = static_cast<uint32_t>(attachments.size());
            FBO_CI.width                   = m_width;
            FBO_CI.height                  = m_height;
            FBO_CI.layers                  = 1;

            if (vkCreateFramebuffer(*m_device, &FBO_CI, nullptr, &m_framebuffer) != VK_SUCCESS) {
                VK_ERROR("Framebuffer::CreateFramebuffer() : failed to create vulkan framebuffer!");
                return false;
            }

            return true;
        }
        bool CreateSampler() {
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
    public:
        VkRenderPassBeginInfo Begin(VkClearValue* clearValues, uint32_t count) {
            VkRenderPassBeginInfo renderPassBeginInfo = Tools::Initializers::RenderPassBeginInfo();

            renderPassBeginInfo.renderPass               = m_renderPass;
            renderPassBeginInfo.framebuffer              = m_framebuffer;
            renderPassBeginInfo.renderArea.extent.width  = m_width;
            renderPassBeginInfo.renderArea.extent.height = m_height;
            renderPassBeginInfo.clearValueCount          = count;
            renderPassBeginInfo.pClearValues             = clearValues;

            return renderPassBeginInfo;
        }

        void Destroy() {
            if (m_renderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(*m_device, m_renderPass, nullptr);
                m_renderPass = VK_NULL_HANDLE;
            }

            if (m_attachments) {
                for (uint32_t i = 0; i < m_countAttach; i++)
                    m_attachments[i].Destroy();
                free(m_attachments);
                m_attachments = nullptr;
            }
        }

        bool ReCreate(uint32_t width, uint32_t height) {
            this->Destroy();

            this->m_width  = width;
            this->m_height = height;

            if (!this->CreateAttachments() ||
                !this->CreateRenderPass()  ||
                !this->CreateFramebuffer() ||
                !this->CreateSampler()
                ) {
                VK_ERROR("Framebuffer::ReCreate() : failed to re-create frame buffer!");
                return false;
            }

            return true;
        }
    public:
        // depth will be auto added to end array of attachments
        static FrameBuffer* Create(
                const Types::Device* device,
                const Types::Swapchain* swapchain,
                const std::vector<VkFormat>& attachments,
                uint32_t width, uint32_t height)
        {
            auto fbo = new FrameBuffer();
            {
                fbo->m_device        = device;
                fbo->m_swapchain     = swapchain;
                fbo->m_countAttach   = attachments.size();
                fbo->m_attachFormats = attachments;
                fbo->m_depthFormat   = Tools::GetDepthFormat(*device);
            }

            if (!fbo->ReCreate(width, height)) {
                VK_ERROR("Framebuffer::Create() : failed to re-create framebuffer!");
                return nullptr;
            }

            return fbo;
        }
    public:
        [[nodiscard]] std::vector<VkDescriptorImageInfo> GetImageDescriptors() const {
            auto descriptors = std::vector<VkDescriptorImageInfo>();

            for (uint32_t i = 0; i < m_countAttach; i++)
                descriptors.push_back(Tools::Initializers::DescriptorImageInfo(
                        m_colorSampler,
                        m_attachments[i].m_view,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

            return descriptors;
        }

        void Free() {
            //TODO:!
        }
    };
}

#endif //EVOVULKAN_FRAMEBUFFER_H
