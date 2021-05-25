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

#include <EvoVulkan/Types/VulkanBuffer.h>

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/RenderPass.h>

namespace EvoVulkan::Core {
    enum class FrameResult {
        Error = 0, Success = 1, OutOfDate = 2
    };

    /*typedef void (__stdcall *KernelRenderFunction)(
            EvoVulkan::Core::VulkanKernel* kernel,
            //EvoVulkan::Types::Swapchain* swapchain,
            EvoVulkan::Types::Device* device,
            VkFence* waitFences,
            VkCommandBuffer* drawBuffers,
            VkSubmitInfo& submitInfo, // copy
            uint32_t currentBuffer
            );*/

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
        VkClearValue m_clearValues[2] {
                { .color = {{0.5f, 0.5f, 0.5f, 1.0f}} },
                { .depthStencil = { 1.0f, 0 } }
        };
    protected:
        bool                       m_hasErrors            = false;
        bool                       m_paused               = false;

        unsigned int               m_newWidth             = 0;
        unsigned int               m_newHeight            = 0;

        unsigned int               m_width                = 0;
        unsigned int               m_height               = 0;

        //VkRenderPass               m_renderPass           = VK_NULL_HANDLE;
        Types::RenderPass          m_renderPass           = { };
        VkPipelineCache            m_pipelineCache        = VK_NULL_HANDLE;

        Types::Device*             m_device               = nullptr;
        Types::Surface*            m_surface              = nullptr;
        Types::Swapchain*          m_swapchain            = nullptr;
        Types::CmdPool*            m_cmdPool              = nullptr;
        Types::DepthStencil*       m_depthStencil         = nullptr;

        Core::DescriptorManager*   m_descriptorManager    = nullptr;

        Types::Synchronization     m_syncs                = {};
        VkSubmitInfo               m_submitInfo           = {};

        unsigned __int8            m_countDCB             = 0;
        //Types::CmdBuffer**         m_drawCmdBuffs         = nullptr;
        VkCommandBuffer*           m_drawCmdBuffs         = nullptr;
        std::vector<VkFence>       m_waitFences           = std::vector<VkFence>();
        std::vector<VkFramebuffer> m_frameBuffers         = std::vector<VkFramebuffer>();
        uint32_t                   m_currentBuffer        = 0;

        VkPipelineStageFlags       m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    private:
        //KernelRenderFunction       m_renderFunction       = nullptr;
    private:
        bool ReCreateFrameBuffers();
        void DestroyFrameBuffers();
    public:
        FrameResult PrepareFrame();
        void        NextFrame();
        FrameResult SubmitFrame();
    public:
        virtual void Render() { /* nothing */ }
        virtual bool BuildCmdBuffers() = 0;
    public:
        [[nodiscard]] inline VkPipelineCache GetPipelineCache() const noexcept { return m_pipelineCache; }

        [[nodiscard]] inline VkCommandBuffer* GetDrawCmdBuffs() const { return m_drawCmdBuffs; }
        [[nodiscard]] inline Types::Device* GetDevice() const { return m_device; }

        [[nodiscard]] inline bool HasErrors() const noexcept { return m_hasErrors; }

        //inline bool SetRenderFunction(KernelRenderFunction drawFunction) {
        //    this->m_renderFunction = drawFunction;
        //    return true;
        //}

        [[nodiscard]] inline Core::DescriptorManager* GetDescriptorManager() const noexcept {
            return m_descriptorManager;
        }

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
                const std::vector<const char*>& instExtensions,
                const std::vector<const char*>& validationLayers
                );

        bool Init(
                const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
                const std::vector<const char*>& deviceExtensions, const bool& enableSampleShading,
                bool vsync
                );
        bool PostInit();

        [[nodiscard]] inline Types::RenderPass GetRenderPass() const { return m_renderPass; }
        [[nodiscard]] inline VkFramebuffer* GetFrameBuffers() { return m_frameBuffers.data(); }
    };
}

#endif //EVOVULKAN_VULKANKERNEL_H
