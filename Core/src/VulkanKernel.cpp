//
// Created by Nikita on 12.04.2021.
//

#define VK_PROTOTYPES

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include "EvoVulkan/VulkanKernel.h"

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
    Tools::VkDebug::Graph("VulkanKernel::Init() : initializing Evo Vulkan kernel...");

    Tools::VkDebug::Graph("VulkanKernel::Init() : create vulkan surface...");
    this->m_surface = Tools::CreateSurface(m_instance, platformCreate);
    if (!m_surface) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed create vulkan surface!");
        return false;
    }

    Tools::VkDebug::Graph("VulkanKernel::Init() : create vulkan logical device...");
    this->m_device = Tools::CreateDevice(
            m_instance,
            m_surface,
            deviceExtensions,
            m_validationEnabled ? m_validationLayers : std::vector<const char*>(),
            enableSampleShading);
    if (!m_device) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed to create logical device!");
        return false;
    }

    if (!m_device->Ready()) {
        Tools::VkDebug::Error("VulkanKernel::Init() : something went wrong! Device isn't ready...");
        return false;
    }

    Tools::VkDebug::Log("VulkanKernel::Init() : count MSAA samples is "
        + std::to_string(m_device->GetMSAASamples()));
    Tools::VkDebug::Log("VulkanKernel::Init() : depth format is "
        + Tools::Convert::format_to_string(m_device->GetDepthFormat()));

    //!=================================================================================================================

    //Tools::LoadVulkanFunctionPointers(m_instance, *m_device);

    if (!m_surface->Init(m_device)) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed to create initialize surface!");
        return false;
    }

    Tools::VkDebug::Graph("VulkanKernel::Init() : create vulkan swapchain...");
    this->m_swapchain = Types::Swapchain::Create(
            m_instance,
            m_surface,
            m_device,
            m_width,
            m_height);
    if (!this->m_swapchain) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed to create swapchain!");
        return false;
    }

    Tools::VkDebug::Log("VulkanKernel::Init() : Evo Vulkan successfully initialized!");

    return true;
}

bool EvoVulkan::Core::VulkanKernel::PostInit() {
    return false;
}

EvoVulkan::Core::VulkanKernel* EvoVulkan::Core::VulkanKernel::Create() {
    Tools::VkDebug::Log("VulkanKernel::Create() : create Evo Vulkan kernel...");

    auto kernel = new VulkanKernel();

    return kernel;
}

bool EvoVulkan::Core::VulkanKernel::Free() {
    Tools::VkDebug::Log("VulkanKernel::Free() : free Evo Vulkan kernel memory...");

    m_swapchain->Destroy();
    m_swapchain->Free();
    m_swapchain = nullptr;

    m_surface->Destroy();
    m_surface->Free();
    m_surface = nullptr;

    m_device->Destroy();
    m_device->Free();
    m_device = nullptr;

    if (m_validationEnabled) {
        Tools::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        this->m_debugMessenger = VK_NULL_HANDLE;
    }

    Tools::DestroyInstance(&m_instance);

    Tools::VkDebug::Log("VulkanKernel::Free() : all resources has been freed! Free kernel pointer...");

    delete this;
    return true;
}
