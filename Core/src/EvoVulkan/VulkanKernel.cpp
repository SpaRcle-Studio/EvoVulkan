//
// Created by Nikita on 12.04.2021.
//

#define VK_PROTOTYPES

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include "EvoVulkan/VulkanKernel.h"
#include <EvoVulkan/Complexes/Shader.h>

bool EvoVulkan::Core::VulkanKernel::PreInit(
        const std::string& appName,
        const std::string& engineName,
        const std::string& glslc,
        const std::vector<const char*>& instExtensions,
        const std::vector<const char*>& validationLayers)
{
    VK_GRAPH("VulkanKernel::PreInit() : pre-initializing Evo Vulkan kernel...");

    m_appName          = appName;
    m_engineName       = engineName;
    m_validationLayers = validationLayers;
    m_instExtensions   = instExtensions;

    Complexes::GLSLCompiler::Instance().Init(glslc);

    //m_instExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
    //m_instExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    // todo : linux/android
	m_instExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

    VK_GRAPH("VulkanKernel::PreInit() : create vulkan instance...");

    std::string supportedExtMsg = "VulkanKernel::PreInit() : supported instance extensions:";
    auto&& supportedExtensions = Tools::GetSupportedInstanceExtensions();
    for (auto&& extension : supportedExtensions) {
        supportedExtMsg.append("\n\t").append(extension);
    }
    VK_LOG(supportedExtMsg);

    m_instance = Types::Instance::Create(
            m_appName,
            m_engineName,
            m_instExtensions,
            m_validationLayers,
            m_validationEnabled);

    if (m_instance == VK_NULL_HANDLE) {
        VK_ERROR("VulkanKernel::PreInit() : failed to create vulkan instance!");
        return false;
    }

    if (m_validationEnabled) {
        m_debugMessenger = Tools::SetupDebugMessenger(*m_instance);
        if (m_debugMessenger == VK_NULL_HANDLE) {
            VK_ERROR("VulkanKernel::PreInit() : failed to setup debug messenger! Try continue...");
        }
    }


    this->m_isPreInitialized = true;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::Init(
        const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
        void* windowHandle,
        const std::vector<const char*>& deviceExtensions,
        const bool& enableSampleShading,
        bool vsync)
{
    VK_GRAPH("VulkanKernel::Init() : initializing Evo Vulkan kernel...");

    //!=============================================[Create surface]====================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan surface...");
    m_surface = Tools::CreateSurface(*m_instance, platformCreate, windowHandle);
    if (!m_surface) {
        VK_ERROR("VulkanKernel::Init() : failed create vulkan surface!");
        return false;
    }

    //!==========================================[Create logical device]================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan logical device...");

    m_device = Tools::CreateDevice(
            m_instance,
            m_surface,
            deviceExtensions,
            m_validationEnabled ? m_validationLayers : std::vector<const char*>(),
            enableSampleShading,
            m_sampleCount > 1,
            m_sampleCount
    );

    if (!m_device) {
        VK_ERROR("VulkanKernel::Init() : failed to create logical device!");
        return false;
    }

    m_sampleCount = m_device->GetMSAASamples();
    if (!m_device->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : something went wrong! Device isn't ready...");
        return false;
    }

    VK_LOG("VulkanKernel::Init() : count MSAA samples is " + std::to_string(m_device->GetMSAASamples()));

    //!=============================================[Create allocator]==================================================

    VK_LOG("VulkanKernel::Init() : create allocator...");
    m_allocator = Memory::Allocator::Create(m_device);

    //!========================================[Create descriptor manager]==============================================

    VK_LOG("VulkanKernel::Init() : create descriptor manager...");
    m_descriptorManager = Core::DescriptorManager::Create(m_device);
    if (!m_descriptorManager) {
        VK_ERROR("VulkanKernel::Init() : failed to create descriptor manager!");
        return false;
    }

    //!=============================================[Init surface]======================================================

    if (!m_surface->Init(m_device)) {
        VK_ERROR("VulkanKernel::Init() : failed to create initialize surface!");
        return false;
    }

    //!===========================================[Create command pool]=================================================

    m_cmdPool = Types::CmdPool::Create(m_device);
    if (!m_cmdPool) {
        VK_ERROR("VulkanKernel::Init() : failed to create command pool!");
        return false;
    }

    //!=============================================[Create swapchain]==================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan swapchain with sizes: width = " +
             std::to_string(m_newWidth) + "; height = " + std::to_string(m_newHeight));

    m_width  = m_newWidth;
    m_height = m_newHeight;

    m_swapchain = Types::Swapchain::Create(
        *m_instance,
        m_surface,
        m_device,
        vsync,
        m_width,
        m_height,
        m_swapchainImages
    );

    if (!m_swapchain) {
        VK_ERROR("VulkanKernel::Init() : failed to create swapchain!");
        return false;
    }

    m_swapchain->SetSampleCount(m_sampleCount);
    if (!m_swapchain->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : swapchain isn't ready!");
        return false;
    }

    VK_LOG("VulkanKernel::Init() : depth format is "
                        + Tools::Convert::format_to_string(m_swapchain->GetDepthFormat()));

    //!=================================================================================================================

    m_isInitialized = true;

    VK_INFO("VulkanKernel::Init() : Evo Vulkan successfully initialized!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::PostInit() {
    VK_INFO("VulkanKernel::PostInit() : post initializing Evo Vulkan kernel...");

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : allocate draw command buffers...");

    m_countDCB = m_swapchain->GetCountImages();

    m_drawCmdBuffs = Tools::AllocateCommandBuffers(
            *m_device,
            Tools::Initializers::CommandBufferAllocateInfo(
                *m_cmdPool,
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                m_countDCB
            )
    );

    if (!m_drawCmdBuffs) {
        VK_ERROR("Vulkan::PostInit() : failed to allocate draw command buffers!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create wait fences...");
    m_waitFences = Tools::CreateFences(*m_device, this->m_countDCB);
    if (m_waitFences.empty()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create wait fences!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create multisample target...");

    m_multisample = Types::MultisampleTarget::Create(
            m_device,
            m_allocator,
            m_cmdPool,
            m_swapchain,
            m_swapchain->GetSurfaceWidth(),
            m_swapchain->GetSurfaceHeight(),
            { m_swapchain->GetColorFormat() },
            m_swapchain->GetSampleCount(),
            true /** depth buffer */
    );

    if (!m_multisample) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create multisample!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create render pass...");
    m_renderPass = Types::CreateRenderPass(
            m_device,
            m_swapchain,
            { } /** color attachment */,
            m_swapchain->GetSampleCount(),
            true /** depth buffer */
    );

    if (!m_renderPass.Ready()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create render pass!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create synchronizations...");
    if (!ReCreateSynchronizations()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create synchronizations!");
        return false;
    }

    //!=================================================================================================================

    m_pipelineCache = Tools::CreatePipelineCache(*m_device);
    if (m_pipelineCache == VK_NULL_HANDLE) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create pipeline cache!");
        return false;
    }

    //!=================================================================================================================

    if (!ReCreateFrameBuffers()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to re-create frame buffers!");
        return false;
    }

    m_isPostInitialized = true;

    VK_LOG("VulkanKernel::PostInit() : call custom on-complete function...");
    if (!OnComplete()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to complete Evo Vulkan!");
        return false;
    }

    VK_INFO("VulkanKernel::PostInit() : Evo Vulkan successfully post-initialized!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::Destroy() {
    VK_LOG("VulkanKernel::Destroy() : free Evo Vulkan kernel memory...");

    if (m_multisample) {
        m_multisample->Destroy();
        m_multisample->Free();
    }

    if (m_descriptorManager)
        this->m_descriptorManager->Free();

    if (!m_frameBuffers.empty())
        this->DestroyFrameBuffers();

    if (m_pipelineCache)
        Tools::DestroyPipelineCache(*m_device, &m_pipelineCache);

    if (m_syncs.IsReady())
        Tools::DestroySynchronization(*m_device, &m_syncs);

    if (m_renderPass.Ready())
        Types::DestroyRenderPass(m_device, &m_renderPass);

    if (!m_waitFences.empty()) {
        Tools::DestroyFences(*m_device, m_waitFences);
        m_waitFences.clear();
    }

    if (m_drawCmdBuffs)
        Tools::FreeCommandBuffers(*m_device, *m_cmdPool, &m_drawCmdBuffs, m_countDCB);

    EVSafeFreeObject(m_swapchain);
    EVSafeFreeObject(m_surface);
    EVSafeFreeObject(m_cmdPool);
    EVSafeFreeObject(m_allocator);
    EVSafeFreeObject(m_device);

    if (m_validationEnabled) {
        Tools::DestroyDebugUtilsMessengerEXT(*m_instance, m_debugMessenger, nullptr);
        this->m_debugMessenger = VK_NULL_HANDLE;
    }

    EVSafeFreeObject(m_instance);

    VK_LOG("VulkanKernel::Destroy() : all resources has been freed!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::ReCreateFrameBuffers() {
    VK_GRAPH("VulkanKernel::ReCreateFrameBuffers() : re-create vulkan frame buffers...");

    if (!m_renderPass.Ready()) {
        VK_ERROR("VulkanKernel::ReCreateFrameBuffers() : render pass in nullptr!");
        return false;
    }

    m_multisample->ReCreate(m_swapchain->GetSurfaceWidth(), m_swapchain->GetSurfaceHeight());

    for (auto & m_frameBuffer : m_frameBuffers)
        vkDestroyFramebuffer(*m_device, m_frameBuffer, nullptr);
    m_frameBuffers.clear();

    std::vector<VkImageView> attachments = {};
    attachments.resize(m_renderPass.m_countAttachments);

    /// Depth/Stencil attachment is the same for all frame buffers
    if (m_swapchain->IsMultisamplingEnabled()) {
        attachments[0] = m_multisample->GetResolve(0);
        /// attachment[1] = swapchain image
        attachments[2] = m_multisample->GetDepth();
    }
    else {
        attachments[1] = m_multisample->GetDepth();
        ///attachments[1] = m_depthStencil->GetImageView();
    }

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = m_renderPass.m_self;
    frameBufferCreateInfo.attachmentCount         = m_renderPass.m_countAttachments;
    frameBufferCreateInfo.pAttachments            = attachments.data();
    frameBufferCreateInfo.width                   = m_swapchain->GetSurfaceWidth();
    frameBufferCreateInfo.height                  = m_swapchain->GetSurfaceHeight();
    frameBufferCreateInfo.layers                  = 1;

    /// Create frame buffers for every swap chain image
    m_frameBuffers.resize(m_countDCB);
    for (uint32_t i = 0; i < m_countDCB; i++) {
        //!attachments[0] = m_swapchain->GetBuffers()[i].m_view;

        attachments[m_swapchain->IsMultisamplingEnabled() ? 1 : 0] = m_swapchain->GetBuffers()[i].m_view;

        auto result = vkCreateFramebuffer(*m_device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[i]);

        if (result != VK_SUCCESS) {
            VK_ERROR("VulkanKernel::ReCreateFrameBuffers() : failed to create vulkan frame buffer! Reason: " +
                Tools::Convert::result_to_description(result));
            return false;
        }
    }

    return true;
}

void EvoVulkan::Core::VulkanKernel::DestroyFrameBuffers() {
    for (auto & m_frameBuffer : m_frameBuffers)
        vkDestroyFramebuffer(*m_device, m_frameBuffer, nullptr);
    m_frameBuffers.clear();
}

EvoVulkan::Core::RenderResult EvoVulkan::Core::VulkanKernel::NextFrame() {
    if (m_paused)
        return EvoVulkan::Core::RenderResult::Success;

    //if (viewUpdated) {
    //    viewUpdated = false;
    //    viewChanged();
    //}

    return this->Render();

    //camera.update(frameTimer);
    //if (camera.moving())
    //    viewUpdated = true;
}

EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::PrepareFrame() {
    // Acquire the next image from the swap chain
    VkResult result = m_swapchain->AcquireNextImage(m_syncs.m_presentComplete, &m_currentBuffer);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        //windowResize();
        VK_LOG("VulkanKernel::PrepareFrame() : window has been resized!");
        return FrameResult::OutOfDate;
    }
    else if (result != VK_SUCCESS) {
        VK_ERROR("VulkanKernel::PrepareFrame() : failed to acquire next image! Reason: " +
            Tools::Convert::result_to_description(result));

        //this->m_hasErrors = true;

        return FrameResult::Error;
    }

    return FrameResult::Success;
    //vkWaitForFences(*m_device, 1, &m_waitFences[m_currentBuffer], VK_TRUE, UINT64_MAX);
    //vkResetFences(*m_device, 1, &m_waitFences[m_currentBuffer]);
}

EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::SubmitFrame() {
    VkResult result = m_swapchain->QueuePresent(m_device->GetGraphicsQueue(), m_currentBuffer, m_syncs.m_renderComplete);
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swap chain is no longer compatible with the surface and needs to be recreated
            //windowResize();
            VK_LOG("VulkanKernel::SubmitFrame() : window has been resized!");
            return FrameResult::OutOfDate;
        }
        else {
            VK_ERROR("VulkanKernel::SubmitFrame() : failed to queue present! Reason: " +
                     Tools::Convert::result_to_description(result));

            if (result == VK_ERROR_DEVICE_LOST) {
                return FrameResult::DeviceLost;
            }

            return FrameResult::Error;
        }
    }

    result = vkQueueWaitIdle(m_device->GetGraphicsQueue());

    if (result != VK_SUCCESS) {
        VK_ERROR("VulkanKernel::SubmitFrame() : failed to queue wait idle! Reason: " +
                 Tools::Convert::result_to_description(result));

        if (result == VK_ERROR_DEVICE_LOST)
            return FrameResult::DeviceLost;

        return FrameResult::Error;
    }

    return FrameResult::Success;
}

bool EvoVulkan::Core::VulkanKernel::ResizeWindow() {
    VK_LOG("VulkanKernel::ResizeWindow() : waiting for a change in the size of the client window...");

    // ждем пока управляющая сторона передаст размеры окна, иначе будет рассинхрон
    while(true) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_newWidth != -1 && m_newHeight != -1)
            break;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    VK_LOG("VulkanKernel::ResizeWindow() : set new sizes: width = " +
        std::to_string(m_newWidth) + "; height = " + std::to_string(m_newHeight));

    if (!m_isPostInitialized) {
        VK_ERROR("VulkanKernel::ResizeWindow() : kernel is not complete!");
        return false;
    }

    vkDeviceWaitIdle(*m_device);

    m_width  = m_newWidth;
    m_height = m_newHeight;

    m_newWidth = -1;
    m_newHeight = -1;

    if (!m_swapchain->SurfaceIsAvailable())
        return true;

    if (!m_swapchain->ReSetup(m_width, m_height, m_swapchainImages)) {
        VK_ERROR("VulkanKernel::ResizeWindow() : failed to re-setup swapchain!");
        return false;
    }

    if (!ReCreateFrameBuffers()) {
        VK_ERROR("VulkanKernel::ResizeWindow() : failed to re-create frame buffers!");
        return false;
    }

    VK_LOG("VulkanKernel::ResizeWindow() : call custom on-resize function...");
    if (!OnResize()) {
        VK_ERROR("VulkanKernel::ResizeWindow() : failed to resize inherited class!");
        return false;
    }

    VK_GRAPH("VulkanKernel::ResizeWindow() : re-create synchronizations...");
    if (!ReCreateSynchronizations()) {
        VK_ERROR("VulkanKernel::ResizeWindow() : failed to re-create synchronizations!");
        return false;
    }

    if (!BuildCmdBuffers()) {
        VK_ERROR("VulkanKernel::ResizeWindow() : failed to build command buffer!");
        return false;
    }

    return true;
}

void EvoVulkan::Core::VulkanKernel::SetMultisampling(uint32_t sampleCount) {
    m_sampleCount = sampleCount;
    VK_ASSERT2(!m_device, "The device is already initialized!");
}

void EvoVulkan::Core::VulkanKernel::SetSize(uint32_t width, uint32_t height)  {
    std::lock_guard<std::mutex> lock(m_mutex);

    VK_LOG("VulkanKernel::SetSize() : set new surface sizes: " + std::to_string(width) + "x" + std::to_string(height));

    m_newWidth  = width;
    m_newHeight = height;

    bool oldPause = m_paused;
    m_paused = m_newHeight == 0 || m_newWidth == 0;
    if (oldPause != m_paused) {
        if (m_paused) {
            VK_LOG("VulkanKernel::SetSize() : window has been collapsed!");
        }
        else
            VK_LOG("VulkanKernel::SetSize() : window has been expanded!");
    }
}

uint32_t EvoVulkan::Core::VulkanKernel::GetCountBuildIterations() const {
    return m_swapchain->GetCountImages();
}

void EvoVulkan::Core::VulkanKernel::SetSwapchainImagesCount(uint32_t count) {
    m_swapchainImages = count;
}

void EvoVulkan::Core::VulkanKernel::SetGUIEnabled(bool enabled)
{
    if ((m_GUIEnabled = enabled)) {
        VK_LOG("VulkanKernel::SetGUIEnabled() : gui was been enabled!");
    }
    else {
        VK_LOG("VulkanKernel::SetGUIEnabled() : gui was been disabled!");
    }
}

bool EvoVulkan::Core::VulkanKernel::ReCreateSynchronizations() {
    if (m_syncs.IsReady()) {
        Tools::DestroySynchronization(*m_device, &m_syncs);
    }

    m_syncs = Tools::CreateSynchronization(*m_device);
    if (!m_syncs.IsReady()) {
        VK_ERROR("VulkanKernel::ReCreateSynchronizations() : failed to create synchronizations!");
        return false;
    }

    /// Set up submit info structure
    /// Semaphores will stay the same during application lifetime
    /// Command buffer submission info is set by each example
    m_submitInfo = Tools::Initializers::SubmitInfo();
    m_submitInfo.pWaitDstStageMask    = &m_submitPipelineStages;
    m_submitInfo.waitSemaphoreCount   = 1;
    m_submitInfo.pWaitSemaphores      = &m_syncs.m_presentComplete;
    m_submitInfo.signalSemaphoreCount = 1;
    m_submitInfo.pSignalSemaphores    = &m_syncs.m_renderComplete;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::SetValidationLayersEnabled(bool value) {
    if (m_isPreInitialized) {
        VK_ERROR("VulkanKernel::SetValidationLayersEnabled() : at this stage it is not possible to set this parameter!");
        return false;
    }

    m_validationEnabled = value;

    return true;
}

void EvoVulkan::Core::VulkanKernel::SetFramebuffersQueue(const std::vector<Complexes::FrameBuffer *> &queue) {
    auto newQueue = std::vector<VkSubmitInfo>();

    VkSubmitInfo submitInfo = Tools::Initializers::SubmitInfo();
    submitInfo.commandBufferCount   = 1;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask    = &m_submitPipelineStages;

    for (uint32_t i = 0; i < queue.size(); ++i) {
        submitInfo.pCommandBuffers   = queue[i]->GetCmdRef();
        submitInfo.pSignalSemaphores = queue[i]->GetSemaphoreRef();

        if (i == 0) {
            submitInfo.pWaitSemaphores = &m_syncs.m_presentComplete;
        }
        else
            submitInfo.pWaitSemaphores = queue[i - 1]->GetSemaphoreRef();

        newQueue.push_back(submitInfo);
    }

    if (!queue.empty()) {
        m_waitSemaphore = queue[queue.size() - 1]->GetSemaphore();
    }
    else
        m_waitSemaphore = VK_NULL_HANDLE;

    m_framebuffersQueue = newQueue;
}

EvoVulkan::Core::DescriptorManager *EvoVulkan::Core::VulkanKernel::GetDescriptorManager() const {
    if (!m_descriptorManager) {
        VK_ERROR("VulkanKernel::GetDescriptorManager() : descriptor manager is nullptr!");
        return nullptr;
    }
    return m_descriptorManager;
}
