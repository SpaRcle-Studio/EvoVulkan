//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_FRAMEBUFFER_H
#define EVOVULKAN_FRAMEBUFFER_H

#include <vulkan/vulkan.h>

#include <vector>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Texture.h>
#include <EvoVulkan/Types/RenderPass.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanTools.h>

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Complexes {
    struct FrameBufferAttachment {
        VkImage             m_image;
        Types::DeviceMemory m_mem;
        VkImageView         m_view;
        VkFormat            m_format;

        Types::Device*      m_device;

        [[nodiscard]] bool Ready() const {
            return m_image  != VK_NULL_HANDLE &&
                   m_mem    != VK_NULL_HANDLE &&
                   m_view   != VK_NULL_HANDLE &&
                   m_device != VK_NULL_HANDLE &&
                   m_format != VK_FORMAT_UNDEFINED;
        }

        void Init() {
            m_image  = VK_NULL_HANDLE;
            m_mem    = {};
            m_view   = VK_NULL_HANDLE;
            m_format = VK_FORMAT_UNDEFINED;

            m_device = VK_NULL_HANDLE;
        }

        void Destroy() {
            if (m_view) {
                vkDestroyImageView(*m_device, m_view, nullptr);
                m_view = VK_NULL_HANDLE;
            }

            if (m_image) {
                vkDestroyImage(*m_device, m_image, nullptr);
                m_image = VK_NULL_HANDLE;
            }

            //vkFreeMemory(m_device, m_mem, nullptr);
            //m_mem = VK_NULL_HANDLE;
            this->m_device->FreeMemory(&m_mem);

            m_device = VK_NULL_HANDLE;
        }
    };

    //!=================================================================================================================

    class FrameBuffer {
    private:
        uint32_t                  m_width             = 0,
                                  m_height            = 0,

                                  m_baseWidth         = 0,
                                  m_baseHeight        = 0,

                                  m_countColorAttach  = 0;

        VkFramebuffer             m_framebuffer       = VK_NULL_HANDLE;
        Types::RenderPass         m_renderPass        = { };

        VkSampler                 m_colorSampler      = VK_NULL_HANDLE;

        std::vector<VkFormat>     m_attachFormats     = {};
        VkFormat                  m_depthFormat       = VK_FORMAT_UNDEFINED;

        float                     m_scale             = 1.f;

        Types::MultisampleTarget* m_multisampleTarget = nullptr;
        Types::Device*            m_device            = nullptr;
        Types::Swapchain*         m_swapchain         = nullptr;
        Types::CmdPool*           m_cmdPool           = nullptr;

        VkRect2D                  m_scissor           = {};
        VkViewport                m_viewport          = {};

        VkCommandBufferBeginInfo  m_cmdBufInfo        = {};

        std::vector<VkClearValue> m_clearValues       = {};
        uint32_t                  m_countClearValues  = 0;

        bool                      m_depthEnabled      = true;
    public:
        /// \Warn Unsafe access! But it's fast.
        FrameBufferAttachment*    m_attachments       = nullptr;

        VkSemaphore               m_semaphore         = VK_NULL_HANDLE;

        VkCommandBuffer           m_cmdBuff           = VK_NULL_HANDLE;
    public:
        operator VkFramebuffer() const { return m_framebuffer; }
    public:
        /// \Warn Slow access! But it's safe.
        [[nodiscard]] VkImageView GetAttachment(const uint32_t id) const {
            if (id >= m_countColorAttach) {
                VK_ERROR("Framebuffer::GetAttachment() : going beyond the array boundaries!");
                return VK_NULL_HANDLE;
            }
            return m_attachments[id].m_view;
        }

        std::vector<Types::Texture*> AllocateColorTextureReferences();
        [[nodiscard]] std::vector<VkDescriptorImageInfo> GetImageDescriptors() const;

        [[nodiscard]] inline VkViewport GetViewport() const { return m_viewport; }
        [[nodiscard]] inline VkRect2D GetScissor() const { return m_scissor; }

        [[nodiscard]] inline Types::RenderPass GetRenderPass() const noexcept { return m_renderPass; }
        [[nodiscard]] inline VkRect2D GetRenderPassArea()      const noexcept { return { VkOffset2D(), { m_width, m_height } }; }
        [[nodiscard]] inline VkCommandBuffer GetCmd()          const noexcept { return m_cmdBuff; }
        [[nodiscard]] const VkClearValue* GetClearValues()     const { return m_clearValues.data(); }
        [[nodiscard]] inline uint32_t GetCountClearValues()    const { return m_countClearValues; }
    private:
        bool CreateAttachments();
        bool CreateRenderPass();
        bool CreateFramebuffer();
        bool CreateSampler();
    public:
        inline void BeginCmd() {
            vkBeginCommandBuffer(m_cmdBuff, &m_cmdBufInfo);
        }
        inline void End() const {
            vkCmdEndRenderPass(m_cmdBuff);
            vkEndCommandBuffer(m_cmdBuff);
        }

        [[nodiscard]] inline VkRenderPassBeginInfo BeginRenderPass(VkClearValue* clearValues, uint32_t countCls) const {
            VkRenderPassBeginInfo renderPassBeginInfo = Tools::Initializers::RenderPassBeginInfo();

            renderPassBeginInfo.renderPass               = m_renderPass.m_self;
            renderPassBeginInfo.framebuffer              = m_framebuffer;
            renderPassBeginInfo.renderArea.extent.width  = m_width;
            renderPassBeginInfo.renderArea.extent.height = m_height;
            renderPassBeginInfo.clearValueCount          = countCls;
            renderPassBeginInfo.pClearValues             = clearValues;

            return renderPassBeginInfo;
        }

        inline void SetViewportAndScissor() const {
            vkCmdSetViewport(m_cmdBuff, 0, 1, &m_viewport);
            vkCmdSetScissor(m_cmdBuff, 0, 1, &m_scissor);
        }
    public:
        void Destroy();
        void Free();
        bool ReCreate(uint32_t width, uint32_t height);
    public:
        // depth will be auto added to end array of attachments
        static FrameBuffer* Create(
                Types::Device* device,
                Types::Swapchain* swapchain,
                Types::CmdPool* pool,
                const std::vector<VkFormat>& colorAttachments,
                uint32_t width, uint32_t height,
                float scale = 1.f);
    };
}

#endif //EVOVULKAN_FRAMEBUFFER_H
