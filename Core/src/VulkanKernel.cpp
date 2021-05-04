//
// Created by Nikita on 12.04.2021.
//

#define VK_PROTOTYPES

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include "EvoVulkan/VulkanKernel.h"

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanConverter.h>

bool EvoVulkan::Core::VulkanKernel::PreInit(
        const std::string& appName,
        const std::string& engineName,
        const std::vector<const char*>& instExtensions,
        const std::vector<const char*>& validationLayers)
{
    Tools::VkDebug::Graph("VulkanKernel::PreInit() : pre-initializing Evo Vulkan kernel...");

    this->m_appName          = appName;
    this->m_engineName       = engineName;
    this->m_validationLayers = validationLayers;

    this->m_instExtensions   = instExtensions;

    //m_instExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
    //m_instExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    // todo : linux/android
	m_instExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

    Tools::VkDebug::Graph("VulkanKernel::PreInit() : create vulkan instance...");

    this->m_instance = Tools::CreateInstance(
            m_appName,
            m_engineName,
            m_instExtensions,
            m_validationLayers,
            m_validationEnabled);

    if (m_instance == VK_NULL_HANDLE) {
        Tools::VkDebug::Error("VulkanKernel::PreInit() : failed to create vulkan instance!");
        return false;
    }

    if (m_validationEnabled) {
        this->m_debugMessenger = Tools::SetupDebugMessenger(m_instance);
        if (m_debugMessenger == VK_NULL_HANDLE) {
            Tools::VkDebug::Error("VulkanKernel::PreInit() : failed to setup debug messenger!");
            return false;
        }
    }


    this->m_isPreInitialized = true;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::Init(
        const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
        const std::vector<const char*>& deviceExtensions,
        const bool& enableSampleShading,
        bool vsync)
{
    VK_GRAPH("VulkanKernel::Init() : initializing Evo Vulkan kernel...");

    //!=============================================[Create surface]====================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan surface...");
    this->m_surface = Tools::CreateSurface(m_instance, platformCreate);
    if (!m_surface) {
        VK_ERROR("VulkanKernel::Init() : failed create vulkan surface!");
        return false;
    }

    //!==========================================[Create logical device]================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan logical device...");
    this->m_device = Tools::CreateDevice(
            m_instance,
            m_surface,
            deviceExtensions,
            m_validationEnabled ? m_validationLayers : std::vector<const char*>(),
            enableSampleShading);
    if (!m_device) {
        VK_ERROR("VulkanKernel::Init() : failed to create logical device!");
        return false;
    }

    if (!m_device->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : something went wrong! Device isn't ready...");
        return false;
    }

    VK_LOG("VulkanKernel::Init() : count MSAA samples is "
        + std::to_string(m_device->GetMSAASamples()));

    //!=============================================[Init surface]======================================================

    if (!m_surface->Init(m_device)) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed to create initialize surface!");
        return false;
    }

    //!===========================================[Create command pool]=================================================

    m_cmdPool = Types::CmdPool::Create(m_device);
    if (!m_cmdPool) {
        VK_ERROR("VulkanKernel::Init() : failed to create command pool!");
        return false;
    }

    //!=============================================[Create swapchain]==================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan swapchain...");
    this->m_swapchain = Types::Swapchain::Create(
            m_instance,
            m_surface,
            m_device,
            vsync,
            m_width,
            m_height);

    if (!this->m_swapchain) {
        VK_ERROR("VulkanKernel::Init() : failed to create swapchain!");
        return false;
    }

    if (!this->m_swapchain->IsReady()) {
        VK_ERROR("VulkanKernel::Init() : swapchain isn't ready!");
        return false;
    }

    Tools::VkDebug::Log("VulkanKernel::Init() : depth format is "
                        + Tools::Convert::format_to_string(m_swapchain->GetDepthFormat()));

    //!=================================================================================================================

    this->m_isInitialized = true;

    VK_INFO("VulkanKernel::Init() : Evo Vulkan successfully initialized!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::PostInit() {
    VK_INFO("VulkanKernel::PostInit() : post initializing Evo Vulkan kernel...");

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create draw command buffers...");
    this->m_countDCB = m_swapchain->GetCountImages();
    this->m_drawCmdBuffs = (Types::CmdBuffer**)malloc(sizeof(Types::CmdBuffer) * m_countDCB);

    for (uint32_t i = 0; i < m_countDCB; i++) {
        this->m_drawCmdBuffs[i] = Types::CmdBuffer::Create(
                m_device,
                m_cmdPool,

                Tools::Initializers::CommandBufferAllocateInfo(
                        *m_cmdPool,
                        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                        1));

        if (!m_drawCmdBuffs[i]->IsReady()) {
            VK_ERROR("VulkanKernel::PostInit() : command buffer isn't ready!");
            return false;
        }
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create wait fences...");
    this->m_waitFences = Tools::CreateFences(*m_device, this->m_countDCB);
    if (m_waitFences.empty()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create wait fences!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create depth stencil...");
    this->m_depthStencil = Types::DepthStencil::Create(m_device, m_swapchain, m_width, m_height);
    if (!m_depthStencil || !m_depthStencil->IsReady()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create depth stencil!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create render pass...");
    this->m_renderPass = Tools::CreateRenderPass(m_device, m_swapchain);
    if (m_renderPass == VK_NULL_HANDLE) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create render pass!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::PostInit() : create synchronizations...");
    this->m_syncs = Tools::CreateSynchronization(*m_device, m_submitPipelineStages);
    if (!m_syncs.IsReady()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create synchronizations!");
        return false;
    }

    //!=================================================================================================================

    this->m_pipelineCache = Tools::CreatePipelineCache(*m_device);
    if (m_pipelineCache == VK_NULL_HANDLE) {
        VK_ERROR("VulkanKernel::PostInit() : failed to create pipeline cache!");
        return false;
    }

    //!=================================================================================================================

    if (!this->ReCreateFrameBuffers()) {
        VK_ERROR("VulkanKernel::PostInit() : failed to re-create frame buffers!");
        return false;
    }

    this->m_isPostInitialized = true;

    VK_INFO("VulkanKernel::PostInit() : Evo Vulkan successfully post-initialized!");

    return true;
}

EvoVulkan::Core::VulkanKernel* EvoVulkan::Core::VulkanKernel::Create() {
    Tools::VkDebug::Log("VulkanKernel::Create() : create Evo Vulkan kernel...");

    auto kernel = new VulkanKernel();

    return kernel;
}

bool EvoVulkan::Core::VulkanKernel::Free() {
    Tools::VkDebug::Log("VulkanKernel::Free() : free Evo Vulkan kernel memory...");

    this->DestroyFrameBuffers();

    Tools::DestroyPipelineCache(*m_device, &m_pipelineCache);
    Tools::DestroySynchronization(*m_device, &m_syncs);

    if (m_renderPass != VK_NULL_HANDLE)
        Tools::DestroyRenderPass(m_device, &m_renderPass);

    if (m_depthStencil) {
        this->m_depthStencil->Destroy();
        this->m_depthStencil->Free();
        this->m_depthStencil = nullptr;
    }

    if (!m_waitFences.empty()) {
        Tools::DestroyFences(*m_device, m_waitFences);
        m_waitFences.clear();
    }

    if (m_drawCmdBuffs) {
        for (uint32_t i = 0; i < m_countDCB; i++) {
            m_drawCmdBuffs[i]->Destroy();
            m_drawCmdBuffs[i]->Free();
            m_drawCmdBuffs[i] = nullptr;
        }
        free(m_drawCmdBuffs);
    }

    if (m_swapchain) {
        m_swapchain->Destroy();
        m_swapchain->Free();
        m_swapchain = nullptr;
    }

    if (m_surface) {
        m_surface->Destroy();
        m_surface->Free();
        m_surface = nullptr;
    }

    if (m_cmdPool) {
        m_cmdPool->Destroy();
        m_cmdPool->Free();
        m_cmdPool = nullptr;
    }

    if (m_device) {
        m_device->Destroy();
        m_device->Free();
        m_device = nullptr;
    }

    if (m_validationEnabled) {
        Tools::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        this->m_debugMessenger = VK_NULL_HANDLE;
    }

    Tools::DestroyInstance(&m_instance);

    Tools::VkDebug::Log("VulkanKernel::Free() : all resources has been freed! Free kernel pointer...");

    delete this;
    return true;
}

bool EvoVulkan::Core::VulkanKernel::ReCreateFrameBuffers() {
    VK_GRAPH("VulkanKernel::ReCreateFrameBuffers() : re-create vulkan frame buffers...");

    if (m_renderPass == VK_NULL_HANDLE) {
        VK_ERROR("VulkanKernel::ReCreateFrameBuffers() : render pass in nullptr!");
        return false;
    }

    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = m_depthStencil->GetImageView();

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext                   = NULL;
    frameBufferCreateInfo.renderPass              = this->m_renderPass;
    frameBufferCreateInfo.attachmentCount         = 2;
    frameBufferCreateInfo.pAttachments            = attachments;
    frameBufferCreateInfo.width                   = m_width;
    frameBufferCreateInfo.height                  = m_height;
    frameBufferCreateInfo.layers                  = 1;

    // Create frame buffers for every swap chain image
    m_frameBuffers.resize(m_countDCB);
    for (uint32_t i = 0; i < m_countDCB; i++) {
        attachments[0] = m_swapchain->GetBuffers()[i].m_view;

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
