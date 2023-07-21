//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANKERNEL_H
#define EVOVULKAN_VULKANKERNEL_H

#include <EvoVulkan/Memory/Allocator.h>

#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Tools/VulkanInsert.h>
#include <EvoVulkan/Tools/SubmitInfo.h>

#include <EvoVulkan/Types/Instance.h>
#include <EvoVulkan/Types/VulkanBuffer.h>
#include <EvoVulkan/Types/RenderPass.h>

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Complexes/Framebuffer.h>

#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Core {
    enum class FrameResult : uint8_t {
        Error,
        Success,
        OutOfDate,
        DeviceLost,
        Dirty,
        Suboptimal
    };

    enum class RenderResult : uint8_t {
        None, Success, Fatal, Error, DeviceLost
    };

    class DLL_EVK_EXPORT VulkanKernel : public Tools::NonCopyable {
    protected:
        VulkanKernel() = default;
        ~VulkanKernel() override = default;

    public:
        virtual bool PreInit(
                const std::string& appName,
                const std::string& engineName,
                const std::string& glslc,
                const std::vector<const char*>& instExtensions,
                const std::vector<const char*>& validationLayers);

        virtual bool Init(
                const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
                void* windowHandle,
                const std::vector<const char*>& deviceExtensions,
                bool enableSampleShading,
                bool vsync);

        virtual bool PostInit();

        virtual FrameResult PrepareFrame();
        virtual RenderResult NextFrame();
        virtual FrameResult SubmitFrame();
        virtual FrameResult QueuePresent();
        virtual FrameResult WaitIdle();

    public:
        EVK_NODISCARD EVK_INLINE VkPipelineCache GetPipelineCache() const noexcept { return m_pipelineCache; }
        EVK_NODISCARD EVK_INLINE VkCommandBuffer* GetDrawCmdBuffs() const { return m_drawCmdBuffs; }
        EVK_NODISCARD EVK_INLINE Types::Device* GetDevice() const { return m_device; }
        EVK_NODISCARD EVK_INLINE Memory::Allocator* GetAllocator() const { return m_allocator; }
        EVK_NODISCARD EVK_INLINE Types::MultisampleTarget* GetMultisampleTarget() const { return m_multisample; }
        EVK_NODISCARD EVK_INLINE Types::CmdPool* GetCmdPool() const { return m_cmdPool; }
        EVK_NODISCARD EVK_INLINE Types::Swapchain* GetSwapchain() const { return m_swapchain; }
        EVK_NODISCARD EVK_INLINE Types::Surface* GetSurface() const { return m_surface; }
        EVK_NODISCARD EVK_INLINE VkInstance GetInstance() const { return *m_instance; }
        EVK_NODISCARD EVK_INLINE bool HasErrors() const noexcept { return m_hasErrors; }
        EVK_NODISCARD EVK_INLINE bool IsDirty() const noexcept { return m_dirty; }
        EVK_NODISCARD EVK_INLINE VkViewport GetViewport() const noexcept { return Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f); }
        EVK_NODISCARD EVK_INLINE const SubmitInfo& GetSubmitInfo() const noexcept { return m_submitInfo; }
        EVK_NODISCARD EVK_INLINE VkRect2D GetScissor()const noexcept { return Tools::Initializers::Rect2D(m_width, m_height, 0, 0); }
        EVK_NODISCARD EVK_INLINE VkRect2D GetRenderArea() const noexcept { return { VkOffset2D(), { m_width, m_height } }; }
        EVK_NODISCARD EVK_INLINE Types::RenderPass GetRenderPass() const noexcept { return m_renderPass; }
        EVK_NODISCARD EVK_INLINE VkFramebuffer* GetFrameBuffers() { return m_frameBuffers.data(); }
        EVK_NODISCARD EVK_INLINE bool IsMultisamplingEnabled() const noexcept { return m_sampleCount > 1; }
        EVK_NODISCARD EVK_INLINE VkSemaphore GetPresentCompleteSemaphore() const noexcept { return m_syncs.m_presentComplete; }
        EVK_NODISCARD EVK_INLINE VkSemaphore GetRenderCompleteSemaphore() const noexcept { return m_syncs.m_renderComplete; }
        EVK_NODISCARD std::vector<VkSemaphore>& GetWaitSemaphores() { return m_submitInfo.waitSemaphores; }

        EVK_NODISCARD uint8_t GetSampleCount() const;
        EVK_NODISCARD EvoVulkan::Types::CmdBuffer* CreateSingleTimeCmd() const;
        EVK_NODISCARD EvoVulkan::Types::CmdBuffer* CreateCmd() const;
        EVK_NODISCARD Core::DescriptorManager* GetDescriptorManager() const;
        EVK_NODISCARD uint32_t GetCountBuildIterations() const;
        EVK_NODISCARD bool IsValidationLayersEnabled() const { return m_validationEnabled; }
        EVK_NODISCARD bool IsSurfaceCollapsed() const { return m_paused; }
        EVK_NODISCARD VkPipelineStageFlags GetSubmitPipelineStages() const { return m_submitPipelineStages; }

        EVK_NODISCARD virtual bool IsWindowValid() const { return true; }

        virtual void PollWindowEvents() { }

        void ClearSubmitQueue();
        void PrintSubmitQueue();
        void AddSubmitQueue(SubmitInfo submitInfo);
        void SetSubmitQueue(std::vector<SubmitInfo>&& queue) { m_submitQueue = std::move(queue); }

        EVK_NODISCARD const std::vector<SubmitInfo>& GetSubmitQueue() const { return m_submitQueue; };

        void SetMultisampling(uint32_t sampleCount);
        void SetSwapchainImagesCount(uint32_t count);

        virtual void SetGUIEnabled(bool enabled);
        virtual bool IsRayTracingRequired() const noexcept { return false; }

        bool SetValidationLayersEnabled(bool value);
        void SetSize(uint32_t width, uint32_t height);
        bool ReCreate(FrameResult reason);

    public:
        virtual bool BuildCmdBuffers() = 0;
        virtual bool Destroy();
        virtual bool OnResize() = 0;
        virtual bool OnComplete() { return true; }

    protected:
        virtual RenderResult Render() { return RenderResult::Fatal; }

    private:
        bool ReCreateFrameBuffers();
        bool ReCreateSynchronizations();
        void DestroyFrameBuffers();

    public:
        uint8_t                    m_countDCB             = 0;
        VkCommandBuffer*           m_drawCmdBuffs         = nullptr;
        std::vector<VkFramebuffer> m_frameBuffers         = std::vector<VkFramebuffer>();

    protected:
        std::recursive_mutex       m_mutex                = std::recursive_mutex();

        bool                       m_hasErrors            = false;
        bool                       m_paused               = false;
        bool                       m_dirty                = false;

        int32_t                    m_newWidth             = -1;
        int32_t                    m_newHeight            = -1;

        uint32_t                   m_width                = 0;
        uint32_t                   m_height               = 0;
        uint32_t                   m_swapchainImages      = 0;
        uint32_t                   m_sampleCount          = 1;

        Types::RenderPass          m_renderPass           = { };
        VkPipelineCache            m_pipelineCache        = VK_NULL_HANDLE;

        Types::Instance*           m_instance             = nullptr;
        Types::Device*             m_device               = nullptr;
        Memory::Allocator*         m_allocator            = nullptr;
        Types::Surface*            m_surface              = nullptr;
        Types::Swapchain*          m_swapchain            = nullptr;
        Types::CmdPool*            m_cmdPool              = nullptr;
        Types::MultisampleTarget*  m_multisample          = nullptr;

        Core::DescriptorManager*   m_descriptorManager    = nullptr;

        Types::Synchronization     m_syncs                = { };
        SubmitInfo                 m_submitInfo           = { };

        std::vector<VkFence>       m_waitFences           = std::vector<VkFence>();
        uint32_t                   m_currentBuffer        = 0;

        std::vector<SubmitInfo>    m_submitQueue          = { };

        VkPipelineStageFlags       m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        bool                       m_GUIEnabled           = false;

    private:
        std::vector<const char*>   m_instExtensions       = {};
        std::vector<const char*>   m_validationLayers     = {};

        std::string                m_appName              = "Unknown";
        std::string                m_engineName           = "NoEngine";

        VkDebugUtilsMessengerEXT   m_debugMessenger       = VK_NULL_HANDLE;

        bool                       m_validationEnabled    = false;

        bool                       m_isPreInitialized     = false;
        bool                       m_isInitialized        = false;
        bool                       m_isPostInitialized    = false;

    };
}

#endif //EVOVULKAN_VULKANKERNEL_H
