//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/VulkanKernel.h>
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
    #ifdef EVK_ANDROID
    #else
	    m_instExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    #endif
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

    m_isPreInitialized = true;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::Init(
        const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
        void* windowHandle,
        const std::vector<const char*>& deviceExtensions,
        bool enableSampleShading,
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

    Types::EvoDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pInstance = m_instance;
    deviceCreateInfo.pSurface = m_surface;
    deviceCreateInfo.enableSampleShading = enableSampleShading;
    deviceCreateInfo.sampleCount = m_sampleCount;
    deviceCreateInfo.multisampling = m_sampleCount > 1;
    deviceCreateInfo.rayTracing = IsRayTracingRequired();
    deviceCreateInfo.extensions = deviceExtensions;
    deviceCreateInfo.validationLayers = m_validationEnabled ? m_validationLayers : std::vector<const char*>();

    m_device = Types::Device::Create(std::move(deviceCreateInfo));
    if (!m_device) {
        VK_ERROR("VulkanKernel::Init() : failed to create evo device!");
        return false;
    }

    if (!m_device->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : something went wrong! Device isn't ready...");
        return false;
    }

    /// так как при создании устройства мы передаем ему желаемое значение,
    /// то нам стоит переспросить у устройства реальное значение, которое оно поддерживает
    m_sampleCount = EVK_MIN(m_device->GetMSAASamples(), m_sampleCount);

    VK_LOG("VulkanKernel::Init() : supported and used count MSAA samples is " + std::to_string(m_device->GetMSAASamples()));

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

    m_newWidth = -1;
    m_newHeight = -1;

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

    if (!m_swapchain->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : swapchain isn't ready!");
        return false;
    }

    VK_LOG("VulkanKernel::Init() : depth format is " + Tools::Convert::format_to_string(m_device->GetDepthFormat()));

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
        GetSampleCount(),
        1 /** layers count */,
        VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
        m_device->GetDepthFormat()
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
            { } /** color attachments */,
            { } /** input attachments */,
            GetSampleCount(),
            VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
            m_device->GetDepthFormat()
    );

    if (!m_renderPass.IsReady()) {
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

    m_dirty = false;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::Destroy() {
    VK_LOG("VulkanKernel::Destroy() : free Evo Vulkan kernel memory...");

    if (m_multisample) {
        m_multisample->Destroy();
        m_multisample->Free();
    }

    if (m_descriptorManager)
        m_descriptorManager->Free();

    if (!m_frameBuffers.empty())
        DestroyFrameBuffers();

    if (m_pipelineCache)
        Tools::DestroyPipelineCache(*m_device, &m_pipelineCache);

    if (m_syncs.IsReady())
        Tools::DestroySynchronization(*m_device, &m_syncs);

    if (m_renderPass.IsReady())
        Types::DestroyRenderPass(m_device, &m_renderPass);

    if (!m_waitFences.empty()) {
        Tools::DestroyFences(*m_device, m_waitFences);
        m_waitFences.clear();
    }

    if (m_drawCmdBuffs) {
        Tools::FreeCommandBuffers(*m_device, *m_cmdPool, &m_drawCmdBuffs, m_countDCB);
    }

    EVSafeFreeObject(m_swapchain);
    EVSafeFreeObject(m_surface);
    EVSafeFreeObject(m_cmdPool);
    EVSafeFreeObject(m_allocator);
    EVSafeFreeObject(m_device);

    if (m_validationEnabled) {
        Tools::DestroyDebugUtilsMessengerEXT(*m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }

    EVSafeFreeObject(m_instance);

    VK_LOG("VulkanKernel::Destroy() : all resources has been freed!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::ReCreateFrameBuffers() {
    VK_GRAPH("VulkanKernel::ReCreateFrameBuffers() : re-create vulkan frame buffers...");

    if (!m_renderPass.IsReady()) {
        VK_ERROR("VulkanKernel::ReCreateFrameBuffers() : render pass in nullptr!");
        return false;
    }

    const uint32_t width = m_swapchain->GetSurfaceWidth();
    const uint32_t height = m_swapchain->GetSurfaceHeight();

    m_multisample->SetSampleCount(GetSampleCount());
    m_multisample->ReCreate(width, height);

    DestroyFrameBuffers();

    Types::DestroyRenderPass(m_device, &m_renderPass);

    m_renderPass = Types::CreateRenderPass(
        m_device,
        m_swapchain,
        { } /** color attachments */,
        { } /** input attachments */,
        GetSampleCount(),
        VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
        m_device->GetDepthFormat()
    );

    /// -----------------------------------------------------------------

    std::vector<VkImageView> attachments = {};
    attachments.resize(m_renderPass.m_countAttachments);

    /// Depth/Stencil attachment is the same for all frame buffers
    if (IsMultisamplingEnabled()) {
        attachments[0] = m_multisample->GetResolve(0);
        attachments[2] = m_multisample->GetDepth();
    }
    else {
        attachments[1] = m_multisample->GetDepth();
    }

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = m_renderPass.m_self;
    frameBufferCreateInfo.attachmentCount         = m_renderPass.m_countAttachments;
    frameBufferCreateInfo.pAttachments            = attachments.data();
    frameBufferCreateInfo.width                   = width;
    frameBufferCreateInfo.height                  = height;
    frameBufferCreateInfo.layers                  = 1;

    /// Create frame buffers for every swap chain image
    m_frameBuffers.resize(m_countDCB);
    for (uint32_t i = 0; i < m_countDCB; ++i) {
        //!attachments[0] = m_swapchain->GetBuffers()[i].m_view;

        attachments[IsMultisamplingEnabled() ? 1 : 0] = m_swapchain->GetBuffers()[i].m_view;

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

    return Render();
}

EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::PrepareFrame() {
    if (m_swapchain->IsDirty()) {
        VK_LOG("VulkanKernel::PrepareFrame() : swapchain is dirty!");
        return FrameResult::Dirty;
    }

    /// Acquire the next image from the swap chain
    VkResult result = m_swapchain->AcquireNextImage(m_syncs.m_presentComplete, &m_currentBuffer);
    /// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        VK_LOG("VulkanKernel::PrepareFrame() : window has been resized!");
        return FrameResult::OutOfDate;
    }
    else if (result == VK_SUBOPTIMAL_KHR) {
        VK_LOG("VulkanKernel::PrepareFrame() : window has been suboptimal!");
        return FrameResult::Suboptimal;
    }
    else if (result != VK_SUCCESS) {
        VK_ERROR("VulkanKernel::PrepareFrame() : failed to acquire next image! Reason: " +
            Tools::Convert::result_to_description(result));

        return FrameResult::Error;
    }

    return FrameResult::Success;
}

EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::QueuePresent() {
    /// TODO: здесь может зависнуть, нужно придумать способ перехвата
    VkResult result = vkQueueWaitIdle(m_device->GetQueues()->GetGraphicsQueue());

    if (result != VK_SUCCESS) {
        VK_ERROR("VulkanKernel::SubmitFrame() : failed to queue wait idle! Reason: " +
                 Tools::Convert::result_to_description(result));

        if (result == VK_ERROR_DEVICE_LOST)
            return FrameResult::DeviceLost;

        return FrameResult::Error;
    }

    return EvoVulkan::Core::FrameResult::Success;
}

EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::WaitIdle() {
    VkResult result = m_swapchain->QueuePresent(m_device->GetQueues()->GetGraphicsQueue(), m_currentBuffer, m_syncs.m_renderComplete);

    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            /// Swap chain is no longer compatible with the surface and needs to be recreated
            VK_LOG("VulkanKernel::WaitIdle() : window has been resized!");
            return FrameResult::OutOfDate;
        }
        else {
            VK_ERROR("VulkanKernel::WaitIdle() : failed to queue present! Reason: " +
                     Tools::Convert::result_to_description(result));

            if (result == VK_ERROR_DEVICE_LOST) {
                return FrameResult::DeviceLost;
            }

            return FrameResult::Error;
        }
    }

    return EvoVulkan::Core::FrameResult::Success;
}

EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::SubmitFrame() {
    if (auto&& result = QueuePresent(); result != FrameResult::Success) {
        return result;
    }

    return WaitIdle();
}

bool EvoVulkan::Core::VulkanKernel::ReCreate(FrameResult reason) {
    VK_LOG("VulkanKernel::ReCreate() : re-create vulkan kernel...");

    if (reason == FrameResult::OutOfDate || reason == FrameResult::Suboptimal) {
        VK_LOG("VulkanKernel::ReCreate() : waiting for a change in the size of the client window...");

        /// ждем пока управляющая сторона передаст размеры окна, иначе будет рассинхрон
        while (true) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            PollWindowEvents();

            if (!IsWindowValid()) {
                VK_LOG("VulkanKernel::ReCreate() : window was closed.");
                break;
            }

            if (m_newWidth != -1 && m_newHeight != -1) {
                break;
            }
        }

        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        VK_LOG("VulkanKernel::ReCreate() : set new sizes: width = " +
               std::to_string(m_newWidth) + "; height = " + std::to_string(m_newHeight));

        if (!m_isPostInitialized) {
            VK_ERROR("VulkanKernel::ReCreate() : kernel is not complete!");
            return false;
        }

        vkDeviceWaitIdle(*m_device);

        m_width = m_newWidth;
        m_height = m_newHeight;

        m_newWidth = -1;
        m_newHeight = -1;
    }
    else {
        vkDeviceWaitIdle(*m_device);
    }

    if (!m_swapchain->SurfaceIsAvailable()) {
        return true;
    }

    if (!m_swapchain->ReSetup(m_width, m_height, m_swapchainImages)) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to re-setup swapchain!");
        return false;
    }

    if (!ReCreateFrameBuffers()) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to re-create frame buffers!");
        return false;
    }

    VK_LOG("VulkanKernel::ReCreate() : call custom on-resize function...");
    if (!OnResize()) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to resize inherited class!");
        return false;
    }

    VK_GRAPH("VulkanKernel::ReCreate() : re-create synchronizations...");
    if (!ReCreateSynchronizations()) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to re-create synchronizations!");
        return false;
    }

    if (!BuildCmdBuffers()) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to build command buffer!");
        return false;
    }

    m_dirty = false;

    return true;
}

void EvoVulkan::Core::VulkanKernel::SetMultisampling(uint32_t sampleCount) {
    if (m_sampleCount == sampleCount) {
        return;
    }

    m_sampleCount = sampleCount;

    if (m_device) {
        if (m_sampleCount == 0) {
            m_sampleCount = m_device->GetMSAASamplesCount();
        }
        else {
            m_sampleCount = EVK_MIN(m_sampleCount, m_device->GetMSAASamplesCount());
        }
    }

    m_dirty = true;
}

void EvoVulkan::Core::VulkanKernel::SetSize(uint32_t width, uint32_t height)  {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    VK_LOG("VulkanKernel::SetSize() : set new surface sizes: " + std::to_string(width) + "x" + std::to_string(height));

    m_newWidth  = width;
    m_newHeight = height;

    bool oldPause = m_paused;
    m_paused = m_newHeight == 0 || m_newWidth == 0;
    if (oldPause != m_paused) {
        if (m_paused) {
            VK_LOG("VulkanKernel::SetSize() : window has been collapsed!");
        }
        else {
            VK_LOG("VulkanKernel::SetSize() : window has been expanded!");
        }
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
    m_submitInfo = SubmitInfo();

    ClearSubmitQueue();

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

void EvoVulkan::Core::VulkanKernel::ClearSubmitQueue() {
    m_submitInfo = SubmitInfo();

    m_submitInfo.SetWaitDstStageMask(m_submitPipelineStages);
    m_submitInfo.signalSemaphores.emplace_back(m_syncs.m_renderComplete);

    m_submitQueue.clear();
}

void EvoVulkan::Core::VulkanKernel::AddSubmitQueue(SubmitInfo submitInfo) {
    m_submitQueue.emplace_back(std::move(submitInfo));
}

EvoVulkan::Core::DescriptorManager *EvoVulkan::Core::VulkanKernel::GetDescriptorManager() const {
    if (!m_descriptorManager) {
        VK_ERROR("VulkanKernel::GetDescriptorManager() : descriptor manager is nullptr!");
        return nullptr;
    }
    return m_descriptorManager;
}

EvoVulkan::Types::CmdBuffer *EvoVulkan::Core::VulkanKernel::CreateCmd() const {
    return Types::CmdBuffer::Create(m_device, m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

EvoVulkan::Types::CmdBuffer* EvoVulkan::Core::VulkanKernel::CreateSingleTimeCmd() const {
    return EvoVulkan::Types::CmdBuffer::BeginSingleTime(m_device, m_cmdPool);
}

uint8_t EvoVulkan::Core::VulkanKernel::GetSampleCount() const {
    return m_sampleCount;
}

void EvoVulkan::Core::VulkanKernel::PrintSubmitQueue() {
    if (m_submitQueue.empty()) {
        return;
    }

    std::string log = "VulkanKernel::PrintSubmitQueue() : \n";

    for (auto&& queue : m_submitQueue) {
        log += "--------------------------------------------------\n";
        for (auto&& wait : queue.waitSemaphores) {
            log += "|\twait semaphore   [" + std::to_string((uint64_t)wait) + "]\n";
        }
        for (auto&& cmd : queue.commandBuffers) {
            log += "|\tcommand buffer   [" + std::to_string((uint64_t)cmd) + "]\n";
        }
        for (auto&& signal : queue.signalSemaphores) {
            log += "|\tsignal semaphore [" + std::to_string((uint64_t)signal) + "]\n";
        }
    }

    log += "--------------------------------------------------";

    VK_LOG(log);
}
