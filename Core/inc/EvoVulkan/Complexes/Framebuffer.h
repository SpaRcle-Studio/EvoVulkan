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
#include <EvoVulkan/Complexes/FrameBufferLayer.h>

#include <EvoVulkan/DescriptorManager.h>

namespace EvoVulkan::Complexes {
    class DLL_EVK_EXPORT FrameBuffer : private Tools::NonCopyable {
        using FrameBufferLayers = std::vector<std::unique_ptr<FrameBufferLayer>>;
        using Attachment = std::unique_ptr<FrameBufferAttachment>;
    protected:
        FrameBuffer() = default;

    public:
        ~FrameBuffer() override;

        /// depth will be auto added to end array of attachments
        static FrameBuffer* Create(
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
                VkFormat depthFormat);

    public:
        bool ReCreate(uint32_t width, uint32_t height);

        void BeginCmd();
        void End() const;
        void SetViewportAndScissor() const;

        void SetSampleCount(uint8_t sampleCount);
        void SetLayersCount(uint32_t layersCount);
        void SetDepthFormat(VkFormat depthFormat);
        void SetDepthAspect(VkImageAspectFlags depthAspect);

        void ClearWaitSemaphores() { m_waitSemaphores.clear(); }
        void ClearSignalSemaphores();

    public:
        EVK_NODISCARD VkImageView GetAttachment(uint32_t layer, uint32_t id) const;

        EVK_NODISCARD std::vector<Types::Texture*> AllocateColorTextureReferences();
        EVK_NODISCARD Types::Texture* AllocateDepthTextureReference();

        EVK_NODISCARD bool IsMultisampleEnabled() const;
        EVK_NODISCARD bool IsDepthEnabled() const;
        EVK_NODISCARD VkSampleCountFlagBits GetSampleCount() const noexcept;

        EVK_NODISCARD EVK_INLINE VkViewport GetViewport() const { return m_viewport; }
        EVK_NODISCARD EVK_INLINE VkRect2D GetScissor() const { return m_scissor; }

        EVK_NODISCARD EVK_INLINE const std::vector<VkFormat>& GetColorFormats() const noexcept { return m_attachFormats; }
        EVK_NODISCARD EVK_INLINE Types::RenderPass GetRenderPass() const noexcept { return m_renderPass; }
        EVK_NODISCARD EVK_INLINE const FrameBufferLayers& GetLayers() const noexcept { return m_layers; }
        EVK_NODISCARD EVK_INLINE VkRect2D GetRenderPassArea() const noexcept { return { VkOffset2D(), { m_width, m_height } }; }
        EVK_NODISCARD EVK_INLINE VkCommandBuffer GetCmd() const noexcept { return *m_cmdBuff; }
        EVK_NODISCARD EVK_INLINE VkCommandBuffer* GetCmdRef() const noexcept { return m_cmdBuff->GetCmdRef(); }
        EVK_NODISCARD EVK_INLINE Types::Device* GetDevice() const noexcept { return m_device; }
        EVK_NODISCARD EVK_INLINE VkSemaphore GetSemaphore() const noexcept { return m_semaphore; }
        EVK_NODISCARD EVK_INLINE VkSemaphore* GetSemaphoreRef() noexcept { return &m_semaphore; }
        EVK_NODISCARD EVK_INLINE Memory::Allocator* GetAllocator() noexcept { return m_allocator; }
        EVK_NODISCARD EVK_INLINE Types::CmdPool* GetCmdPool() noexcept { return m_cmdPool; }
        EVK_NODISCARD EVK_INLINE VkExtent2D GetExtent2D() noexcept { return VkExtent2D { m_width, m_height }; }
        EVK_NODISCARD EVK_INLINE uint32_t GetCountClearValues() const { return m_countClearValues; }
        EVK_NODISCARD EVK_INLINE VkImageAspectFlags GetDepthAspect() const { return m_depthAspect; }
        EVK_NODISCARD EVK_INLINE VkFormat GetDepthFormat() const { return m_depthFormat; }
        EVK_NODISCARD const VkClearValue* GetClearValues() const { return m_clearValues.data(); }
        EVK_NODISCARD std::vector<VkSemaphore>& GetWaitSemaphores() { return m_waitSemaphores; }
        EVK_NODISCARD std::vector<VkSemaphore>& GetSignalSemaphores() { return m_signalSemaphores; }

        EVK_NODISCARD VkRenderPassBeginInfo BeginRenderPass(VkClearValue* clearValues, uint32_t countCls, uint32_t layer) const;

    private:
        void DeInitialize();

        bool CreateAttachments();
        bool CreateRenderPass();
        bool CreateFramebuffer();
        bool CreateSampler();

    private:
        Attachment                m_depthAttachment;
        VkImageAspectFlags        m_depthAspect        = VK_IMAGE_ASPECT_NONE;
        VkFormat                  m_depthFormat        = VK_FORMAT_UNDEFINED;

        FrameBufferLayers         m_layers             = { };
        uint32_t                  m_layersCount        = 0;
        std::vector<VkFormat>     m_attachFormats      = { };

        VkSemaphore               m_semaphore          = VK_NULL_HANDLE;

        uint32_t                  m_width              = 0;
        uint32_t                  m_height             = 0;

        uint32_t                  m_baseWidth          = 0;
        uint32_t                  m_baseHeight         = 0;

        float_t                   m_scale              = 1.f;

        VkSampler                 m_colorSampler       = VK_NULL_HANDLE;
        Types::RenderPass         m_renderPass         = { };

        Types::Device*            m_device             = nullptr;
        Memory::Allocator*        m_allocator          = nullptr;
        Types::Swapchain*         m_swapchain          = nullptr;
        Types::CmdPool*           m_cmdPool            = nullptr;
        Core::DescriptorManager*  m_descriptorManager  = nullptr;
        Types::CmdBuffer*         m_cmdBuff            = nullptr;

        VkRect2D                  m_scissor            = { };
        VkViewport                m_viewport           = { };

        VkCommandBufferBeginInfo  m_cmdBufInfo         = { };

        std::vector<VkSemaphore>  m_waitSemaphores     = { };
        std::vector<VkSemaphore>  m_signalSemaphores   = { };

        std::vector<VkClearValue> m_clearValues        = { };
        uint32_t                  m_countClearValues   = 0;

        uint8_t                   m_sampleCount        = 0;
        uint8_t                   m_currentSampleCount = 0;

    };
}

#endif //EVOVULKAN_FRAMEBUFFER_H
