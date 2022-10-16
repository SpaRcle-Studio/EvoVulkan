//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_FRAMEBUFFER_H
#define EVOVULKAN_FRAMEBUFFER_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Texture.h>
#include <EvoVulkan/Types/RenderPass.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/Image.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanTools.h>

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Complexes {
    class DLL_EVK_EXPORT FrameBufferAttachment : public Tools::NonCopyable {
    public:
        FrameBufferAttachment() = default;
        ~FrameBufferAttachment() override = default;

        FrameBufferAttachment(FrameBufferAttachment&& attachment) noexcept {
            m_image = std::exchange(attachment.m_image, {});
            m_view = std::exchange(attachment.m_view, {});
            m_format = std::exchange(attachment.m_format, {});
            m_device = std::exchange(attachment.m_device, {});
            m_allocator = std::exchange(attachment.m_allocator, {});
        }

        FrameBufferAttachment& operator=(FrameBufferAttachment&& attachment) noexcept {
            m_image = std::exchange(attachment.m_image, {});
            m_view = std::exchange(attachment.m_view, {});
            m_format = std::exchange(attachment.m_format, {});
            m_device = std::exchange(attachment.m_device, {});
            m_allocator = std::exchange(attachment.m_allocator, {});

            return *this;
        }

    public:
        void Init();
        void Destroy();

        EVK_NODISCARD bool Ready() const;

    public:
        Types::Image m_image = Types::Image();
        VkImageView m_view = VK_NULL_HANDLE;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        Types::Device* m_device = nullptr;
        EvoVulkan::Memory::Allocator* m_allocator = nullptr;

    };

    //!=================================================================================================================

    class DLL_EVK_EXPORT FrameBuffer : private Tools::NonCopyable {
    protected:
        FrameBuffer() = default;
        ~FrameBuffer() override = default;

    public:
        /// depth will be auto added to end array of attachments
        static FrameBuffer* Create(
                Types::Device* device,
                EvoVulkan::Memory::Allocator* allocator,
                Core::DescriptorManager* manager,
                Types::Swapchain* swapchain,
                Types::CmdPool* pool,
                const std::vector<VkFormat>& colorAttachments,
                uint32_t width, uint32_t height,
                float scale = 1.f,
                uint8_t multisample = 0,
                bool depth = true);

        operator VkFramebuffer() const { return m_framebuffer; }

    public:
        void Destroy();
        void Free();
        bool ReCreate(uint32_t width, uint32_t height);

        void BeginCmd();
        void End() const;
        void SetViewportAndScissor() const;

    public:
        /// \Warn Slow access! But it's safe.
        EVK_NODISCARD VkImageView GetAttachment(uint32_t id) const;

        EVK_NODISCARD std::vector<Types::Texture*> AllocateColorTextureReferences();
        EVK_NODISCARD Types::Texture* AllocateDepthTextureReference();
        EVK_NODISCARD std::vector<VkDescriptorImageInfo> GetImageDescriptors() const;

        EVK_NODISCARD bool IsMultisampleEnabled() const;
        EVK_NODISCARD VkSampleCountFlagBits GetSampleCount() const noexcept;

        EVK_NODISCARD EVK_INLINE VkViewport GetViewport() const { return m_viewport; }
        EVK_NODISCARD EVK_INLINE VkRect2D GetScissor() const { return m_scissor; }

        EVK_NODISCARD EVK_INLINE Types::RenderPass GetRenderPass() const noexcept { return m_renderPass; }
        EVK_NODISCARD EVK_INLINE VkRect2D GetRenderPassArea() const noexcept { return { VkOffset2D(), { m_width, m_height } }; }
        EVK_NODISCARD EVK_INLINE VkCommandBuffer GetCmd() const noexcept { return m_cmdBuff; }
        EVK_NODISCARD EVK_INLINE VkSemaphore GetSemaphore() const noexcept { return m_semaphore; }
        EVK_NODISCARD EVK_INLINE VkCommandBuffer* GetCmdRef() noexcept { return &m_cmdBuff; }
        EVK_NODISCARD EVK_INLINE VkSemaphore* GetSemaphoreRef() noexcept { return &m_semaphore; }
        EVK_NODISCARD EVK_INLINE uint32_t GetCountClearValues() const { return m_countClearValues; }
        EVK_NODISCARD const VkClearValue* GetClearValues() const { return m_clearValues.data(); }

        EVK_NODISCARD VkRenderPassBeginInfo BeginRenderPass(VkClearValue* clearValues, uint32_t countCls) const;

    private:
        bool CreateAttachments();
        bool CreateRenderPass();
        bool CreateFramebuffer();
        bool CreateSampler();

    public:
        /// \Warn Unsafe access! But it's fast.
        FrameBufferAttachment*    m_attachments        = nullptr;

        VkSemaphore               m_semaphore          = VK_NULL_HANDLE;

        VkCommandBuffer           m_cmdBuff            = VK_NULL_HANDLE;

    private:
        uint32_t                  m_width              = 0;
        uint32_t                  m_height             = 0;

        uint32_t                  m_baseWidth          = 0;
        uint32_t                  m_baseHeight         = 0;

        float_t                   m_scale              = 1.f;

        uint32_t                  m_countColorAttach   = 0;

        VkSampler                 m_colorSampler       = VK_NULL_HANDLE;
        VkFramebuffer             m_framebuffer        = VK_NULL_HANDLE;
        Types::RenderPass         m_renderPass         = {};

        std::vector<VkFormat>     m_attachFormats      = {};
        VkFormat                  m_depthFormat        = VK_FORMAT_UNDEFINED;

        Types::MultisampleTarget* m_multisampleTarget  = nullptr;
        Types::Device*            m_device             = nullptr;
        Memory::Allocator*        m_allocator          = nullptr;
        Types::Swapchain*         m_swapchain          = nullptr;
        Types::CmdPool*           m_cmdPool            = nullptr;
        Core::DescriptorManager*  m_descriptorManager  = nullptr;

        VkRect2D                  m_scissor            = {};
        VkViewport                m_viewport           = {};

        VkCommandBufferBeginInfo  m_cmdBufInfo         = {};

        std::vector<VkClearValue> m_clearValues        = {};
        uint32_t                  m_countClearValues   = 0;

        bool                      m_depthEnabled       = true;
        uint8_t                   m_sampleCount        = 0;

    };
}

#endif //EVOVULKAN_FRAMEBUFFER_H
