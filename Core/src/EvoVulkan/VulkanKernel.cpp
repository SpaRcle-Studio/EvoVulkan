//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/VulkanKernel.h>
#include <EvoVulkan/Complexes/Shader.h>

EvoVulkan::Core::VulkanKernel::~VulkanKernel() = default;

bool EvoVulkan::Core::VulkanKernel::PreInit(
        const std::string& appName,
        const std::string& engineName,
        const std::string& glslc,
        const std::vector<const char*>& instExtensions,
        const std::vector<const char*>& validationLayers)
{
    VK_GRAPH("VulkanKernel::PreInit() : pre-initializing Evo Vulkan kernel...");

    uint32_t instanceVersion = VK_API_VERSION_1_0;
    auto&& FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (FN_vkEnumerateInstanceVersion) {
        FN_vkEnumerateInstanceVersion(&instanceVersion);
    }

    uint32_t major = VK_VERSION_MAJOR(instanceVersion);
    uint32_t minor = VK_VERSION_MINOR(instanceVersion);
    uint32_t patch = VK_VERSION_PATCH(instanceVersion);

    VK_LOG("VulkanKernel::PreInit() : vulkan version: " + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));

    m_appName          = appName;
    m_engineName       = engineName;
    m_instExtensions   = instExtensions;

    Complexes::GLSLCompiler::Instance().Init(glslc);

    //m_instExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
    //m_instExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    #ifdef EVK_ANDROID
    #else
	    //m_instExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    #endif
#endif

    VK_GRAPH("VulkanKernel::PreInit() : creating vulkan instance...");

    std::string supportedExtMsg = "VulkanKernel::PreInit() : supported instance extensions:";
    auto&& supportedExtensions = Tools::GetSupportedInstanceExtensions();
    for (auto&& extension : supportedExtensions) {
        supportedExtMsg.append("\n\t").append(extension);
    }
    VK_LOG(supportedExtMsg);

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::string supportedLayersMsg = "VulkanKernel::PreInit() : supported instance layers:";
    for (auto&& layer : availableLayers) {
        supportedLayersMsg.append("\n\t").append(layer.layerName);
    }
    VK_LOG(supportedLayersMsg);

    for (auto&& layer : validationLayers) {
        if (std::find_if(availableLayers.begin(), availableLayers.end(), [&layer](auto&& data) {
            return strcmp(layer, data.layerName) == 0;
        }) != availableLayers.end()) {
            m_validationLayers.emplace_back(layer);
        }
        else {
            VK_WARN("VulkanKernel::PreInit() : layer " + std::string(layer) + " is not supported!");
        }
    }

    if (m_validationLayers.empty() && m_validationLayersEnabled) {
        m_validationLayersEnabled = false;
        VK_WARN("VulkanKernel::PreInit() : validation is disabled!");
    }

    m_instance = Types::Instance::Create(
            m_appName,
            m_engineName,
            m_instExtensions,
            m_validationLayers,
            m_gpuAssistEnabled,
            m_validationLayersEnabled,
            m_validationDebugEnabled);

    if (m_instance == VK_NULL_HANDLE) {
        VK_ERROR("VulkanKernel::PreInit() : failed to create vulkan instance!");
        return false;
    }

    if (m_validationDebugEnabled) {
        m_debugMessenger = Tools::SetupDebugMessenger(*m_instance);
        if (m_debugMessenger == VK_NULL_HANDLE) {
            VK_ERROR("VulkanKernel::PreInit() : failed to setup debug messenger! Trying to continue...");
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
        bool enableDynamicRendering,
        bool enableMultisampling,
        bool vsync)
{
    VK_GRAPH("VulkanKernel::Init() : initializing Evo Vulkan kernel...");

    //!=============================================[Create surface]====================================================

    if (windowHandle) {
        VK_GRAPH("VulkanKernel::Init() : creating vulkan surface...");
        m_surface = Tools::CreateSurface(*m_instance, platformCreate, windowHandle);
        if (!m_surface) {
            VK_ERROR("VulkanKernel::Init() : failed to create vulkan surface!");
            return false;
        }
    }
    else {
        VK_WARN("VulkanKernel::Init() : window handle is not set! Can't create a surface.");
    }

    //!==========================================[Create logical device]================================================

    VK_GRAPH("VulkanKernel::Init() : creating vulkan logical device...");

    Types::EvoDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pInstance = m_instance;
    deviceCreateInfo.pSurface = m_surface;
    deviceCreateInfo.enableSampleShading = enableSampleShading;
    deviceCreateInfo.sampleCount = m_sampleCount;
    deviceCreateInfo.multisampling = enableMultisampling;
    deviceCreateInfo.rayTracing = IsRayTracingRequired();
    deviceCreateInfo.extensions = deviceExtensions;
    deviceCreateInfo.dynamicRendering = enableDynamicRendering;
    deviceCreateInfo.validationLayers = m_validationLayersEnabled ? m_validationLayers : std::vector<const char*>();

    m_device = Types::Device::Create(std::move(deviceCreateInfo));
    if (!m_device) {
        VK_ERROR("VulkanKernel::Init() : failed to create evo device!");
        return false;
    }

    if (auto&& pQueues = m_device->GetQueues(); !pQueues || !pQueues->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : something went wrong! Family queues aren't ready...");
        return false;
    }

    if (!m_device->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : something went wrong! Device isn't ready...");
        return false;
    }

    /// так как при создании устройства мы передаем ему желаемое значение,
    /// то нам стоит переспросить у устройства реальное значение, которое оно поддерживает
    m_sampleCount = EVK_MIN(m_device->GetMSAASamples(), m_sampleCount);

    VK_LOG("VulkanKernel::Init() : supported and used MSAA sample count is " + std::to_string(m_device->GetMSAASamples()));

    //!=============================================[Create allocator]==================================================

    VK_LOG("VulkanKernel::Init() : creating allocator...");
    m_allocator = Memory::Allocator::Create(m_device);

    //!========================================[Create descriptor manager]==============================================

    VK_LOG("VulkanKernel::Init() : creating descriptor manager...");
    m_descriptorManager = Core::DescriptorManager::Create(m_device);
    if (!m_descriptorManager) {
        VK_ERROR("VulkanKernel::Init() : failed to create descriptor manager!");
        return false;
    }

    //!=============================================[Init surface]======================================================

    if (m_surface) {
        if (!m_surface->Init(m_device)) {
            VK_ERROR("VulkanKernel::Init() : failed to create initialize surface!");
            return false;
        }
    }

    //!===========================================[Create command pool]=================================================

    m_cmdPool = Types::CmdPool::Create(m_device, m_device->GetQueues()->GetGraphicsIndex());
    if (!m_cmdPool) {
        VK_ERROR("VulkanKernel::Init() : failed to create command pool!");
        return false;
    }

    m_computeCmdPool = Types::CmdPool::Create(m_device, m_device->GetQueues()->GetComputeIndex());
    if (!m_computeCmdPool) {
        VK_ERROR("VulkanKernel::Init() : failed to create compute command pool!");
        return false;
    }

    //!=============================================[Create swapchain]==================================================

    VK_GRAPH("VulkanKernel::Init() : creating vulkan swapchain with sizes: width = " +
             std::to_string(m_newWidth) + "; height = " + std::to_string(m_newHeight));

    m_width  = m_newWidth;
    m_height = m_newHeight;

    m_newWidth = -1;
    m_newHeight = -1;

    if (m_surface) {
        m_swapchain = Types::Swapchain::Create(
            *m_instance,
            m_surface,
            m_device,
            m_cmdPool,
            vsync,
            m_width,
            m_height,
            m_requiredSwapchainImages
        );

        if (!m_swapchain) {
            VK_ERROR("VulkanKernel::Init() : failed to create swapchain!");
            return false;
        }

        m_swapchainImages = m_swapchain->GetCountImages();

        if (!m_swapchain->IsReady()) {
            VK_ERROR("VulkanKernel::Init() : swapchain isn't ready!");
            return false;
        }
    }

    m_frameCmdPools.resize(m_swapchainImages);
    VK_LOG("VulkanKernel::Init() : creating " + std::to_string(m_swapchainImages) + " frame command pools...");
    for (size_t i = 0; i < m_swapchainImages; ++i) {
        m_frameCmdPools[i] = Types::CmdPool::Create(m_device, m_device->GetQueues()->GetGraphicsIndex());
        if (!m_frameCmdPools[i]) {
            VK_ERROR("VulkanKernel::Init() : failed to create frame command pool for " + std::to_string(i + 1) + " frame!");
            return false;
        }
    }

    VK_LOG("VulkanKernel::Init() : depth format is " + Tools::Convert::format_to_string(m_device->GetDepthFormat()));

    //!=================================================================================================================

    m_isInitialized = true;

    VK_INFO("VulkanKernel::Init() : Evo Vulkan successfully initialized!");

    return true;
}

/**
 * @brief Performs post-initialization of the Vulkan kernel by allocating and creating runtime resources required for rendering.
 *
 * This configures and allocates compute and draw command buffers, an offscreen semaphore, multisample target (if a swapchain exists),
 * synchronization primitives for frames, a pipeline cache, and framebuffers. On success the kernel is marked as post-initialized,
 * the OnComplete() callback is invoked, and the kernel dirty flag is cleared.
 *
 * @return true if all post-initialization steps completed successfully; `false` if any allocation or creation failed.
 */
bool EvoVulkan::Core::VulkanKernel::PostInit() {
    VK_INFO("VulkanKernel::PostInit() : post initializing Evo Vulkan kernel...");

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : allocate draw command buffers...");

    m_countCCB = 1;
    m_computeCmdBuffers = Tools::AllocateCommandBuffers(
        *m_device,
        Tools::Initializers::CommandBufferAllocateInfo(*m_computeCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_countCCB)
    );

    if (!m_computeCmdBuffers) {
        VK_ERROR("Vulkan::PostInit() : failed to allocate compute command buffers!");
        return false;
    }

    if (!m_offscreenSemaphore) {
        m_offscreenSemaphore = Tools::CreateVulkanSemaphore(*m_device);
        if (!m_offscreenSemaphore) {
            VK_ERROR("VulkanKernel::PostInit() : failed to create offscreen semaphore!");
            return false;
        }
    }

    if (!ReCreateDCBuffers()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to re-create draw command buffers!");
        return false;
    }

    //!=================================================================================================================

    if (m_swapchain) {
        VK_GRAPH("VulkanKernel::PostInit() : creating multisample target...");

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
    }

    //!=================================================================================================================

    //if (m_swapchain) {
    //    VK_GRAPH("VulkanKernel::PostInit() : creating render pass...");
    //    m_renderPass = Types::CreateRenderPass(
    //            m_device,
    //            m_swapchain,
    //            { } /** color attachments */,
    //            { } /** input attachments */,
    //            GetSampleCount(),
    //            VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
    //            m_device->GetDepthFormat()
    //    );

    //    if (!m_renderPass.IsReady()) {
    //        VK_ERROR("VulkanKernel::PostInit() : failed to create render pass!");
    //        return false;
    //    }
    //}

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : creating synchronizations...");
    if (!ReCreateSynchronizations(FrameResult::None)) {
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

    VK_LOG("VulkanKernel::PostInit() : calling custom on-complete function...");
    if (!OnComplete()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to complete Evo Vulkan!");
        return false;
    }

    VK_INFO("VulkanKernel::PostInit() : Evo Vulkan successfully post-initialized!");

    m_dirty = false;

    return true;
}

/**
 * @brief Releases all Vulkan resources owned by the kernel and resets internal state.
 *
 * This cleans up device, swapchain, command pools, framebuffers, synchronization objects,
 * descriptor manager, pipeline cache, allocator, validation debug messenger, and the Vulkan
 * instance so the kernel holds no active Vulkan resources afterward.
 *
 * @return bool `true` if cleanup completed successfully.
 */
bool EvoVulkan::Core::VulkanKernel::Destroy() {
    VK_LOG("VulkanKernel::Destroy() : freeing Evo Vulkan kernel memory...");

    if (m_device) {
        m_device->WaitQueuesIdle();
    }

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

    DestroySynchronizations(FrameResult::None);

    if (m_renderPass.IsReady())
        Types::DestroyRenderPass(m_device, &m_renderPass);

    if (m_device) {
        Tools::DestroyFences(*m_device, m_waitFences);
    }

    DestroyDCBuffers();

    if (m_computeCmdBuffers) {
        Tools::FreeCommandBuffers(*m_device, *m_computeCmdPool, &m_computeCmdBuffers, m_countCCB);
    }

    if (m_device) {
        Tools::DestroyVulkanSemaphore(*m_device, &m_offscreenSemaphore);
    }

    EVSafeFreeObject(m_swapchain);
    EVSafeFreeObject(m_surface);
    EVSafeFreeObject(m_cmdPool);
    EVSafeFreeObject(m_computeCmdPool);

    for (auto& pCmdPool : m_frameCmdPools) {
        EVSafeFreeObject(pCmdPool);
    }
    m_frameCmdPools.clear();

    EVSafeFreeObject(m_allocator);
    EVSafeFreeObject(m_device);

    if (m_validationLayersEnabled && m_instance) {
        Tools::DestroyDebugUtilsMessengerEXT(*m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }

    EVSafeFreeObject(m_instance);

    VK_LOG("VulkanKernel::Destroy() : all resources are freed!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::ReCreateFrameBuffers() {
    if (!m_swapchain) {
        return true;
    }

    VK_GRAPH("VulkanKernel::ReCreateFrameBuffers() : re-creating vulkan frame buffers...");

    const uint32_t width = m_swapchain->GetSurfaceWidth();
    const uint32_t height = m_swapchain->GetSurfaceHeight();

    m_multisample->SetSampleCount(GetSampleCount());
    m_multisample->ReCreate(width, height);

    DestroyFrameBuffers();

    if (m_renderPass.IsReady()) {
        Types::DestroyRenderPass(m_device, &m_renderPass);
    }

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

/**
 * @brief Advances the renderer to the next frame or skips rendering when paused.
 *
 * If the kernel is paused, no rendering is performed and the call completes successfully.
 *
 * @return EvoVulkan::Core::RenderResult `Success` if rendering was skipped due to pause, otherwise the result returned by `Render()`.
 */
EvoVulkan::Core::RenderResult EvoVulkan::Core::VulkanKernel::NextFrame() {
    if (m_paused) {
        return EvoVulkan::Core::RenderResult::Success;
    }

    return Render();
}

/**
 * @brief No-op placeholder for waiting on the current frame's in-flight fence.
 *
 * Currently disabled; this function does not wait on any Vulkan fences.
 */
void EvoVulkan::Core::VulkanKernel::WaitFences() {
    //if (m_device && !m_waitFences.empty()) {
    //    vkWaitForFences(*m_device, 1, &m_waitFences[m_currentBuffer], VK_TRUE, UINT64_MAX);
    //}
}


/**
 * @brief Blocks until the logical Vulkan device becomes idle.
 *
 * If no logical device is available, this function returns immediately.
 */
void EvoVulkan::Core::VulkanKernel::WaitDeviceIdle() {
    if (m_device) {
        vkDeviceWaitIdle(*m_device);
    }
}

/**
 * @brief Blocks until every per-frame in-flight fence becomes signaled.
 *
 * Waits indefinitely for each frame's in-flight fence to be signaled using the Vulkan device.
 * If there are no frames or no fences, this function returns immediately.
 */
void EvoVulkan::Core::VulkanKernel::WaitAllFences() {
    //if (m_device && !m_waitFences.empty()) {
    //    vkWaitForFences(*m_device, static_cast<uint32_t>(m_waitFences.size()), m_waitFences.data(), VK_TRUE, UINT64_MAX);
    //}

    for (auto& frame : m_frames) {
        vkWaitForFences(*m_device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
    }

    //if (!GetInFlightFences().empty()) {
    //    vkWaitForFences(*GetDevice(), GetInFlightFences().size(), GetInFlightFences().data(), VK_TRUE, UINT64_MAX);
    //}
}

/**
 * @brief Prepares the next frame for rendering by acquiring a swapchain image and synchronizing per-frame fences.
 *
 * Waits for the current frame's in-flight fence to become free, acquires the next swapchain image, waits
 * on the image's in-flight fence if that image is already in use, and associates the acquired image with
 * the current frame's in-flight fence.
 *
 * @return FrameResult
 * - FrameResult::Success if an image was acquired and is ready for rendering.
 * - FrameResult::Suboptimal if the swapchain returned VK_SUBOPTIMAL_KHR (image acquired but presentation may need recreation).
 * - FrameResult::OutOfDate if the swapchain is out of date (window resized) and must be recreated.
 * - FrameResult::Error if acquiring the next image failed for another reason.
 */
EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::PrepareFrame() {
    EVK_TRACY_ZONE;

    if (m_swapchain->IsDirty()) {
        VK_LOG("VulkanKernel::PrepareFrame() : swapchain is dirty!");
    }

    FrameSync& frame = m_frames[m_frameIndex];

    // 1. Ждём, пока этот sync-slot освободится
    {
        EVK_TRACY_ZONE_N("Wait for in-flight fence");
        EVK_TRACY_ZONE_COLOR(0xffa500);
        vkWaitForFences(*m_device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
    }

    // 2. Получаем image от swapchain
    VkResult result = m_swapchain->AcquireNextImage(frame.imageAvailable, &m_imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        VK_LOG("VulkanKernel::PrepareFrame() : window has been resized!");
        return FrameResult::OutOfDate;
    }
    else if (result == VK_SUBOPTIMAL_KHR) {
        VK_LOG("VulkanKernel::PrepareFrame() : window has been suboptimal!");
    }
    else if (result != VK_SUCCESS) {
        VK_ERROR("VulkanKernel::PrepareFrame() : failed to acquire next image! Reason: " + Tools::Convert::result_to_description(result));
        return FrameResult::Error;
    }

    // 3. Если image уже используется — ждём fence
    if (m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE) {
        EVK_TRACY_ZONE_N("Wait for image in-flight fence");
        EVK_TRACY_ZONE_COLOR(0xff4500);
        vkWaitForFences(*m_device, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
    }

    // 4. Привязываем image к текущему frame fence
    m_imagesInFlight[m_imageIndex] = frame.inFlightFence;

    return result == VK_SUBOPTIMAL_KHR ? FrameResult::Suboptimal : FrameResult::Success;
}

/**
 * @brief Waits until the graphics queue becomes idle.
 *
 * Blocks until the device's graphics queue is idle or an error occurs.
 *
 * @return FrameResult `Success` if the queue became idle, `DeviceLost` if the device was lost, `Error` for any other failure.
 */
EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::WaitIdle() {
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

/**
 * @brief Submits any allocated compute command buffers and blocks until the compute queue is idle.
 *
 * If no compute command buffers have been allocated, the function logs a warning and returns without
 * submitting or waiting.
 */
void EvoVulkan::Core::VulkanKernel::WaitComputeIdle() {
    if (m_countCCB == 0) {
        VK_WARN("VulkanKernel::WaitComputeIdle() : no compute command buffers allocated!");
        return;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = m_countCCB;
    submitInfo.pCommandBuffers = m_computeCmdBuffers;

    vkQueueSubmit(m_device->GetQueues()->GetComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_device->GetQueues()->GetComputeQueue());
}

/**
 * @brief Presents the currently acquired swapchain image to the display.
 *
 * Uses the current frame's presentation semaphore to submit the image to the graphics queue
 * and handles common swapchain/results states.
 *
 * @return FrameResult
 * - `Success` if the image was presented successfully.
 * - `Suboptimal` if presentation succeeded but the swapchain is suboptimal and should be recreated soon.
 * - `OutOfDate` if the swapchain is out of date (window resize) and must be recreated before further presentation.
 * - `DeviceLost` if the Vulkan device was lost.
 * - `Error` for other presentation failures.
 */
EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::QueuePresent() {
    /// Use m_currentImage for semaphore to match the acquired image index
    //VkResult result = m_swapchain->QueuePresent(m_device->GetQueues()->GetGraphicsQueue(), m_currentImage, m_frameSyncs[m_currentImage].m_renderComplete);

    FrameSync& frame = m_frames[m_frameIndex];
    VkResult result = m_swapchain->QueuePresent(m_device->GetQueues()->GetGraphicsQueue(), m_imageIndex, frame.renderFinished);

    if (result == VK_SUBOPTIMAL_KHR) {
        /// Reset the flag for the next frame
       // m_imageAcquiredThisFrame = false;
        return FrameResult::Suboptimal;
    }

    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            /// Swap chain is no longer compatible with the surface and needs to be recreated
            VK_LOG("VulkanKernel::WaitIdle() : window has been resized!");
            /// Reset the flag for the next frame
            //m_imageAcquiredThisFrame = false;
            return FrameResult::OutOfDate;
        }
        else {
            VK_ERROR("VulkanKernel::WaitIdle() : failed to queue present! Reason: " +
                     Tools::Convert::result_to_description(result));

            if (result == VK_ERROR_DEVICE_LOST) {
                /// Reset the flag for the next frame
               // m_imageAcquiredThisFrame = false;
                return FrameResult::DeviceLost;
            }

            /// Reset the flag for the next frame
            //m_imageAcquiredThisFrame = false;
            return FrameResult::Error;
        }
    }

    /// Reset the flag for the next frame after successful present
   // m_imageAcquiredThisFrame = false;

    return EvoVulkan::Core::FrameResult::Success;
}

/**
 * @brief Presents the current frame to the display.
 *
 * @return FrameResult Presentation outcome: `Success` on successful presentation,
 * `Suboptimal` when the swapchain is usable but not optimal, `OutOfDate` when the
 * swapchain must be recreated, `DeviceLost` if the device was lost, or `Error`
 * for other failures.
 */
EvoVulkan::Core::FrameResult EvoVulkan::Core::VulkanKernel::SubmitFrame() {
    //if (auto&& result = WaitIdle(); result != FrameResult::Success) {
    //    return result;
    //}

    return QueuePresent();
}

/**
 * @brief Recreates the swapchain and all dependent GPU resources to match the current surface state.
 *
 * When the provided @p reason indicates an out-of-date or suboptimal swapchain, this method waits
 * for the host to supply a new window size before proceeding. It ensures device idleness and
 * recreates the swapchain, draw command buffers, framebuffers, synchronization primitives,
 * and rebuilds command buffers. Calls the overrideable OnResize() hook and clears the kernel dirty flag on success.
 *
 * @param reason The cause for recreation (e.g., OutOfDate or Suboptimal) which influences waiting and synchronization behavior.
 * @return true if recreation completed successfully and the kernel state is consistent; `false` if any step failed.
 */
bool EvoVulkan::Core::VulkanKernel::ReCreate(FrameResult reason) {
    /// Reset the flag when recreating swapchain
  //  m_imageAcquiredThisFrame = false;
    VK_LOG("VulkanKernel::ReCreate() : re-creating vulkan kernel...");

    if (reason == FrameResult::OutOfDate || reason == FrameResult::Suboptimal) {
        VK_INFO("VulkanKernel::ReCreate() : waiting for a change in the size of the client window...");

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
            VK_ERROR("VulkanKernel::ReCreate() : kernel is not ready!");
            return false;
        }

        WaitAllFences();
        vkDeviceWaitIdle(*m_device);

        m_width = m_newWidth;
        m_height = m_newHeight;

        m_newWidth = -1;
        m_newHeight = -1;
    }
    else {
        WaitAllFences();
        vkDeviceWaitIdle(*m_device);
    }

    if (!m_swapchain->SurfaceIsAvailable()) {
        return true;
    }

    if (!m_swapchain->ReSetup(m_width, m_height, m_swapchainImages)) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to re-setup swapchain!");
        return false;
    }

    m_swapchainImages = m_swapchain->GetCountImages();

    if (m_countDCB != m_swapchainImages) {
        VK_LOG("VulkanKernel::ReCreate() : swapchain images count changed from " +
               std::to_string(m_countDCB) + " to " + std::to_string(m_swapchainImages));

        if (!ReCreateDCBuffers()) {
            VK_ERROR("VulkanKernel::ReCreate() : failed to re-create draw command buffers!");
            return false;
        }
    }

    if (!ReCreateFrameBuffers()) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to re-create frame buffers!");
        return false;
    }

    VK_LOG("VulkanKernel::ReCreate() : calling custom on-resize function...");
    if (!OnResize()) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to resize inherited class!");
        return false;
    }

    VK_GRAPH("VulkanKernel::ReCreate() : re-creating synchronizations...");
    if (!ReCreateSynchronizations(reason)) {
        VK_ERROR("VulkanKernel::ReCreate() : failed to re-create synchronizations!");
       // m_imageAcquiredThisFrame = false;  ///< Reset flag on error
        return false;
    }

    /// Reset the flag after recreating swapchain
  //  m_imageAcquiredThisFrame = false;

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

    VK_LOG("VulkanKernel::SetSize() : setting new surface sizes: " + std::to_string(width) + "x" + std::to_string(height));

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
    m_requiredSwapchainImages = count;
    m_dirty = true;
}

/**
 * @brief Enable or disable the kernel's GUI.
 *
 * Sets the internal flag that controls whether GUI rendering and related
 * behaviors are active.
 *
 * @param enabled `true` to enable the GUI, `false` to disable it.
 */
void EvoVulkan::Core::VulkanKernel::SetGUIEnabled(bool enabled)
{
    if ((m_GUIEnabled = enabled)) {
        VK_LOG("VulkanKernel::SetGUIEnabled() : gui is enabled!");
    }
    else {
        VK_LOG("VulkanKernel::SetGUIEnabled() : gui is disabled!");
    }
}

/**
 * @brief Destroys per-frame synchronization primitives and optionally clears frame records.
 *
 * Destroys each frame's image-available and render-finished semaphores. Destroys each frame's
 * in-flight fence and clears the internal frame list unless the provided reason indicates a
 * swapchain resize/out-of-date condition (FrameResult::OutOfDate or FrameResult::Suboptimal),
 * in which case fences are kept and frame records are preserved for reuse during recreation.
 *
 * @param reason The context for destruction; if `FrameResult::OutOfDate` or `FrameResult::Suboptimal`,
 *               in-flight fences are preserved and `m_frames` is not cleared to allow swapchain
 *               recreation to reattach existing fences.
void EvoVulkan::Core::VulkanKernel::DestroySynchronizations(FrameResult reason) {
    for (auto&& frame : m_frames) {
        Tools::DestroyVulkanSemaphore(*GetDevice(), &frame.imageAvailable);
        Tools::DestroyVulkanSemaphore(*GetDevice(), &frame.renderFinished);
        if (reason != FrameResult::OutOfDate && reason != FrameResult::Suboptimal) {
            Tools::DestroyVulkanFence(*GetDevice(), &frame.inFlightFence);
        }
    }

    if (reason != FrameResult::OutOfDate && reason != FrameResult::Suboptimal) {
        m_frames.clear();
    }

    //for (auto&& sync : m_frameSyncs) {
    //    if (sync.IsReady()) {
    //        Tools::DestroySynchronization(*m_device, &sync);
    //    }
    //}
    //m_frameSyncs.clear();

    //for (auto&& imageFence : m_imagesInFlight) {
    //    Tools::DestroyVulkanFence(*GetDevice(), &imageFence);
    //}
    //m_imagesInFlight.clear();
}

/**
 * @brief (Re)creates per-frame synchronization objects and initializes submission state.
 *
 * Recreates the per-frame semaphores and fences used for image acquisition and presentation,
 * resizes the image-in-flight tracking array to match the current swapchain image count,
 * resets the submit info structure, and clears the queued submissions.
 *
 * @param reason Controls fence recreation behavior during swapchain-related transitions
 *               (e.g., when the swapchain is OutOfDate or Suboptimal, existing in-flight
 *               fences may be preserved).
 * @return true if all synchronization objects were created and submission state initialized successfully, false otherwise.
 */
bool EvoVulkan::Core::VulkanKernel::ReCreateSynchronizations(FrameResult reason) {
    DestroySynchronizations(reason);

    m_frames.resize(GetMaxFramesInFlight());
    for (auto& frame : m_frames) {
        frame.imageAvailable = Tools::CreateVulkanSemaphore(*m_device);
        frame.renderFinished = Tools::CreateVulkanSemaphore(*m_device);

        if (reason != FrameResult::OutOfDate && reason != FrameResult::Suboptimal || frame.inFlightFence == VK_NULL_HANDLE) {
            frame.inFlightFence = Tools::CreateVulkanFence(*m_device, VK_FENCE_CREATE_SIGNALED_BIT);
        }

        if (!frame.imageAvailable || !frame.renderFinished || !frame.inFlightFence) {
            VK_ERROR("VulkanKernel::ReCreateSynchronizations() : failed to create frame synchronization objects!");
            return false;
        }
    }


    m_imagesInFlight.resize(m_swapchain ? m_swapchain->GetCountImages() : 0, VK_NULL_HANDLE);
    //for (auto& imageFence : m_imagesInFlight) {
    //    imageFence = Tools::CreateVulkanFence(*m_device, VK_FENCE_CREATE_SIGNALED_BIT);
    //}


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

    m_validationLayersEnabled = value;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::SetValidationDebugEnabled(bool value) {
    if (m_isPreInitialized) {
        VK_ERROR("VulkanKernel::SetValidationDebugEnabled() : at this stage it is not possible to set this parameter!");
        return false;
    }

    m_validationDebugEnabled = value;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::SetGPUAssistEnabled(bool value) {
    if (m_isPreInitialized) {
        VK_ERROR("VulkanKernel::SetGPUAssistEnabled() : at this stage it is not possible to set this parameter!");
        return false;
    }

    m_gpuAssistEnabled = value;

    return true;
}

void EvoVulkan::Core::VulkanKernel::ClearSubmitQueue() {
    m_submitInfo = SubmitInfo();

    m_submitInfo.SetWaitDstStageMask(m_submitPipelineStages);
    //m_submitInfo.signalSemaphores.emplace_back(m_syncs.m_renderComplete);

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

bool EvoVulkan::Core::VulkanKernel::ReCreateDCBuffers() {
    Tools::DestroyFences(*m_device, m_waitFences);

    DestroyDCBuffers();

    m_countDCB = m_swapchain ? m_swapchain->GetCountImages() : 0;
    m_drawCmdBuffs.resize(m_countDCB);

    for (uint32_t i = 0; i < m_countDCB; ++i) {
        m_drawCmdBuffs[i] = Tools::AllocateCommandBuffer(
            *m_device,
            Tools::Initializers::CommandBufferAllocateInfo(*m_frameCmdPools[i], VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1)
        );

        if (!m_drawCmdBuffs[i]) {
            VK_ERROR("VulkanKernel::ReCreateDCBuffers() : failed to allocate draw command buffer for " + std::to_string(i + 1) + " frame!");
            return false;
        }
    }

    //!=================================================================================================================

    if (m_countDCB > 0) {
        VK_GRAPH("VulkanKernel::ReCreateDCBuffers() : creating wait fences...");
        m_waitFences = Tools::CreateFences(*m_device, m_countDCB);
        if (m_waitFences.empty()) {
            VK_ERROR("VulkanKernel::ReCreateDCBuffers() : failed to create wait fences!");
            return false;
        }
    }

    return true;
}

/**
 * @brief Frees and clears the per-swapchain-image draw command buffers.
 *
 * Releases any allocated draw command buffers using their corresponding per-frame
 * command pools and clears the internal draw command buffer list.
 *
 * @return bool `true` after buffers have been freed and the list cleared.
 */
bool EvoVulkan::Core::VulkanKernel::DestroyDCBuffers() {
    for (uint32_t i = 0; i < m_drawCmdBuffs.size(); ++i) {
        if (m_drawCmdBuffs[i]) {
            Tools::FreeCommandBuffer(*m_device, *m_frameCmdPools[i], &m_drawCmdBuffs[i]);
        }
    }
    m_drawCmdBuffs.clear();
    return true;
}

/**
 * @brief Returns the maximum number of frames that may be processed concurrently.
 *
 * @return uint8_t The maximum concurrent frames-in-flight (3).
 */
uint8_t EvoVulkan::Core::VulkanKernel::GetMaxFramesInFlight() const noexcept {
    return 3;
}