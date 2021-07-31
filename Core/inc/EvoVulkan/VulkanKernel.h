//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANKERNEL_H
#define EVOVULKAN_VULKANKERNEL_H

/*
    1. helper
    2. types
    3. tools
    4. core
 */

#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Tools/VulkanInsert.h>

#include <EvoVulkan/Types/VulkanBuffer.h>

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/RenderPass.h>
#include <EvoVulkan/Complexes/Framebuffer.h>

#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Core {
    enum class FrameResult {
        Error = 0, Success = 1, OutOfDate = 2
    };

    class VulkanKernel {
    public:
        VulkanKernel(const VulkanKernel&) = delete;
    protected:
        VulkanKernel()  = default;
        ~VulkanKernel() = default;
    private:
        std::vector<const char*>   m_instExtensions       = {};
        std::vector<const char*>   m_validationLayers     = {};

        std::string                m_appName              = "Unknown";
        std::string                m_engineName           = "NoEngine";

        VkInstance                 m_instance             = VK_NULL_HANDLE;

        VkDebugUtilsMessengerEXT   m_debugMessenger       = VK_NULL_HANDLE;

        bool                       m_validationEnabled    = false;

        bool                       m_isPreInitialized     = false;
        bool                       m_isInitialized        = false;
        bool                       m_isPostInitialized    = false;
    protected:
        //VkClearValue m_clearValues[3] {
        //        { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
        //        { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
        //        { .depthStencil = { 1.0f, 0 } }
        //};
    protected:
        bool                       m_multisampling        = false;
        uint32_t                   m_sampleCount          = 1;

        bool                       m_hasErrors            = false;
        bool                       m_paused               = false;

        unsigned int               m_newWidth             = 0;
        unsigned int               m_newHeight            = 0;

        unsigned int               m_width                = 0;
        unsigned int               m_height               = 0;

        Types::RenderPass          m_renderPass           = { };
        VkPipelineCache            m_pipelineCache        = VK_NULL_HANDLE;

        Types::Device*             m_device               = nullptr;
        Types::Surface*            m_surface              = nullptr;
        Types::Swapchain*          m_swapchain            = nullptr;
        Types::CmdPool*            m_cmdPool              = nullptr;
        Types::MultisampleTarget*  m_multisample          = nullptr;

        Core::DescriptorManager*   m_descriptorManager    = nullptr;

        /// optional. Maybe nullptr
        VkSemaphore                m_waitSemaphore        = VK_NULL_HANDLE;
        Types::Synchronization     m_syncs                = {};
        VkSubmitInfo               m_submitInfo           = {};

        std::vector<VkFence>       m_waitFences           = std::vector<VkFence>();
        uint32_t                   m_currentBuffer        = 0;

        std::vector<VkSubmitInfo>  m_framebuffersQueue    = {};

        VkPipelineStageFlags       m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        bool                       m_GUIEnabled           = false;
    public:
        uint8_t                    m_countDCB             = 0;
        VkCommandBuffer*           m_drawCmdBuffs         = nullptr;
        std::vector<VkFramebuffer> m_frameBuffers         = std::vector<VkFramebuffer>();
    private:
        bool ReCreateFrameBuffers();
        void DestroyFrameBuffers();
    public:
        FrameResult PrepareFrame();
        void        NextFrame();
        FrameResult SubmitFrame();
    protected:
        virtual void Render() { /* nothing */ }
    public:
        virtual bool BuildCmdBuffers() = 0;
    public:
        [[nodiscard]] inline uint32_t GetCountBuildIterations() noexcept { return 3; }
        [[nodiscard]] inline VkPipelineCache GetPipelineCache() const noexcept { return m_pipelineCache; }

        [[nodiscard]] inline VkCommandBuffer* GetDrawCmdBuffs() const { return m_drawCmdBuffs; }
        [[nodiscard]] inline Types::Device* GetDevice() const { return m_device; }
        [[nodiscard]] inline Types::MultisampleTarget* GetMultisampleTarget() const { return m_multisample; }
        [[nodiscard]] inline Types::CmdPool* GetCmdPool() const { return m_cmdPool; }
        [[nodiscard]] inline Types::Swapchain* GetSwapchain() const { return m_swapchain; }
        [[nodiscard]] inline VkInstance GetInstance() const { return m_instance; }

        [[nodiscard]] inline bool HasErrors() const noexcept { return m_hasErrors; }

        //inline bool SetRenderFunction(KernelRenderFunction drawFunction) {
        //    this->m_renderFunction = drawFunction;
        //    return true;
        //}

        [[nodiscard]] inline VkViewport GetViewport()   const noexcept { return Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f); }
        [[nodiscard]] inline VkRect2D   GetScissor()    const noexcept { return Tools::Initializers::Rect2D(m_width, m_height, 0, 0);                       }
        [[nodiscard]] inline VkRect2D   GetRenderArea() const noexcept { return { VkOffset2D(), { m_width, m_height } };                                    }

        [[nodiscard]] inline Types::RenderPass GetRenderPass() const noexcept { return m_renderPass; }
        [[nodiscard]] inline VkFramebuffer* GetFrameBuffers() { return m_frameBuffers.data(); }

        [[nodiscard]] inline bool MultisamplingEnabled() const noexcept { return m_multisampling; }

        [[nodiscard]] inline Core::DescriptorManager* GetDescriptorManager() const {
            if (!m_descriptorManager) {
                Tools::VkDebug::Error("VulkanKernel::GetDescriptorManager() : descriptor manager is nullptr!");
                return nullptr;
            }
            return m_descriptorManager;
        }

        void SetFramebuffersQueue(const std::vector<Complexes::FrameBuffer*>& queue) {
            auto newQueue = std::vector<VkSubmitInfo>();

            VkSubmitInfo submitInfo = Tools::Initializers::SubmitInfo();
            submitInfo.commandBufferCount   = 1;
            submitInfo.waitSemaphoreCount   = 1;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pWaitDstStageMask    = &m_submitPipelineStages;

            for (uint32_t i = 0; i < queue.size(); i++) {
                submitInfo.pCommandBuffers   = queue[i]->GetCmdRef();
                submitInfo.pSignalSemaphores = queue[i]->GetSemaphoreRef();

                if (i == 0)
                    submitInfo.pWaitSemaphores = &m_syncs.m_presentComplete;
                else
                    submitInfo.pWaitSemaphores = queue[i - 1]->GetSemaphoreRef();

                newQueue.push_back(submitInfo);
            }

            if (!queue.empty())
                m_waitSemaphore = queue[queue.size() - 1]->GetSemaphore();
            else
                m_waitSemaphore = VK_NULL_HANDLE;

            this->m_framebuffersQueue = newQueue;
        }

        void SetMultisampling(const uint32_t& sampleCount);

        void SetGUIEnabled(bool enabled) { this->m_GUIEnabled = enabled; }

        inline bool SetValidationLayersEnabled(const bool& value) {
            if (m_isPreInitialized) {
                Tools::VkDebug::Error("VulkanKernel::SetValidationLayersEnabled() : at this stage it is not possible to set this parameter!");
                return false;
            }

            this->m_validationEnabled = value;

            return true;
        }
        inline void SetSize(unsigned int width, unsigned int height) {
            this->m_newWidth  = width;
            this->m_newHeight = height;

            bool oldPause = m_paused;
            m_paused = m_newHeight == 0 || m_newWidth == 0;
            if (oldPause != m_paused) {
                if (m_paused)
                    VK_LOG("VulkanKernel::SetSize() : window has been collapsed!");
                else
                    VK_LOG("VulkanKernel::SetSize() : window has been expend!");
            }
        }
        bool ResizeWindow();
    public:
        //static VulkanKernel* Create();
        virtual bool Destroy();
        virtual bool OnResize() = 0;
        virtual bool OnComplete() { return true; }
    public:
        bool PreInit(
                const std::string& appName,
                const std::string& engineName,
                const std::string& glslc,
                const std::vector<const char*>& instExtensions,
                const std::vector<const char*>& validationLayers
                );

        bool Init(
                const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
                const std::vector<const char*>& deviceExtensions, const bool& enableSampleShading,
                bool vsync
                );
        bool PostInit();
    };
}

#endif //EVOVULKAN_VULKANKERNEL_H
