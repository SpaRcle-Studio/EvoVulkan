//
// Created by Nikita on 12.04.2021.
//

#define VK_PROTOTYPES

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include "EvoVulkan/VulkanKernel.h"

bool EvoVulkan::Core::VulkanKernel::PreInit(
        const std::string& appName,
        const std::string& engineName,
        const std::vector<const char*>& validationLayers)
{
    Tools::VkDebug::Graph("VulkanKernel::PreInit() : pre-initializing Evo Vulkan kernel...");

    this->m_appName          = appName;
    this->m_engineName       = engineName;
    this->m_validationLayers = validationLayers;

    m_instExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
    m_instExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
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
        Tools::VkDebug::Error("VulkanKernel::PreInit() : failed create vulkan instance!");
        return false;
    }

    this->m_isPreInitialized = true;

    return true;
}

bool EvoVulkan::Core::VulkanKernel::Init(
        const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
        const std::vector<const char*>& deviceExtensions)
{
    Tools::VkDebug::Graph("VulkanKernel::Init() : initializing Evo Vulkan kernel...");

    Tools::VkDebug::Graph("VulkanKernel::Init() : create vulkan surface...");
    this->m_surface = Tools::CreateSurface(m_instance, platformCreate);
    if (!m_surface) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed create vulkan surface!");
        return false;
    }

    Tools::VkDebug::Graph("VulkanKernel::Init() : create vulkan logical device...");
    this->m_device = Tools::CreateDevice(m_instance, m_surface, deviceExtensions);
    if (!m_device) {
        Tools::VkDebug::Error("VulkanKernel::Init() : failed create logical device!");
        return false;
    }

    return true;
}

bool EvoVulkan::Core::VulkanKernel::PostInit() {
    return false;
}

EvoVulkan::Core::VulkanKernel *EvoVulkan::Core::VulkanKernel::Create() {
    Tools::VkDebug::Graph("VulkanKernel::Create() : create Evo Vulkan kernel...");

    auto kernel = new VulkanKernel();

    return kernel;
}

bool EvoVulkan::Core::VulkanKernel::Free() {
    Tools::VkDebug::Graph("VulkanKernel::Free() : free Evo Vulkan kernel memory...");

    Tools::DestroyInstance(&m_instance);

    delete this;
    return true;
}
