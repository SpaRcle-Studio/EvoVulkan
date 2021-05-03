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
        const bool& enableSampleShading)
{
    VK_GRAPH("VulkanKernel::Init() : initializing Evo Vulkan kernel...");

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan surface...");
    this->m_surface = Tools::CreateSurface(m_instance, platformCreate);
    if (!m_surface) {
        VK_ERROR("VulkanKernel::Init() : failed create vulkan surface!");
        return false;
    }

    //!=================================================================================================================

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

    //!=================================================================================================================

    if (!m_surface->Init(m_device)) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed to create initialize surface!");
        return false;
    }

    //!=================================================================================================================

    m_cmdPool = Types::CmdPool::Create(m_device);
    if (!m_cmdPool) {
        VK_ERROR("VulkanKernel::Init() : failed to create command pool!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::Init() : create setup command buffer...");
    this->m_setupCmdBuff = Types::CmdBuffer::Create(
            m_device, m_cmdPool,
            Tools::Initializers::CommandBufferAllocateInfo(
                    *m_cmdPool,
                    VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1));
    if (!m_setupCmdBuff) {
        VK_ERROR("VulkanKernel::Init() : failed to create setup command buffer!");
        return false;
    }

    //!=================================================================================================================

    VK_GRAPH("VulkanKernel::Init() : create vulkan swapchain...");
    this->m_swapchain = Types::Swapchain::Create(
            m_instance,
            m_surface,
            m_device,
            m_setupCmdBuff,
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

    //!=================================================================================================================

    this->m_isInitialized = true;

    VK_INFO("VulkanKernel::Init() : Evo Vulkan successfully initialized!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::PostInit() {
    //Tools::VkDebug::Log("VulkanKernel::PostInit() : depth format is "
    //                    + Tools::Convert::format_to_string(m_swapchain->GetDepthFormat()));

    this->m_isPostInitialized = true;

    return true;
}

EvoVulkan::Core::VulkanKernel* EvoVulkan::Core::VulkanKernel::Create() {
    Tools::VkDebug::Log("VulkanKernel::Create() : create Evo Vulkan kernel...");

    auto kernel = new VulkanKernel();

    return kernel;
}

bool EvoVulkan::Core::VulkanKernel::Free() {
    Tools::VkDebug::Log("VulkanKernel::Free() : free Evo Vulkan kernel memory...");

    if (m_setupCmdBuff) {
        m_setupCmdBuff->Destroy();
        m_setupCmdBuff->Free();
        m_setupCmdBuff = nullptr;
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
