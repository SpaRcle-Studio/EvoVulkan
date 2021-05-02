//
// Created by Nikita on 12.04.2021.
//

#include "EvoVulkan/Types/Swapchain.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>

EvoVulkan::Types::Swapchain* EvoVulkan::Types::Swapchain::Create(
        VkInstance const &instance,
        EvoVulkan::Types::Surface *surface,
        EvoVulkan::Types::Device *device,
        unsigned int width,
        unsigned int height)
{
    Tools::VkDebug::Graph("Swapchain::Create() : create vulkan swapchain...");

    auto* swapchain = new Swapchain();
    {
        swapchain->m_instance = instance;
        swapchain->m_device = device;
        swapchain->m_surface = surface;

        swapchain->m_swapchain = VK_NULL_HANDLE;
    }

    if (!swapchain->ReSetup(width, height)) {
        Tools::VkDebug::Error("Swapchain::Create() : failed to setup swapchain...");
        return nullptr;
    }

    Tools::VkDebug::Graph("Swapchain::Create() : swapchain successfully created!");

    return swapchain;
}

bool EvoVulkan::Types::Swapchain::ReSetup(unsigned int width, unsigned int height) {
    Tools::VkDebug::Graph("Swapchain::ReSetup() : re-setup vulkan swapchain...");

    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCaps = {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_device, *m_surface, &surfCaps) != VK_SUCCESS) {
        Tools::VkDebug::Error("Swapchain::ReSetup() : failed get physical device surface capabilities!");
        return false;
    }

    //! TODO: see VS example
    if (surfCaps.currentExtent.width != width || surfCaps.currentExtent.height != height) {
        Tools::VkDebug::Error("Swapchain::ReSize() : swap chain size different!");
        return false;
    }

    Tools::VkDebug::Graph("Swapchain::ReSetup() : get present mode...");
    this->m_presentMode = Tools::GetPresentMode(*m_device, *m_surface);

    // Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;

    VkSurfaceTransformFlagsKHR preTransform = {};
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        preTransform = surfCaps.currentTransform;

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.pNext                    = NULL;
    swapchainCI.surface                  = *m_surface;
    swapchainCI.minImageCount            = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat              = m_surface->GetColorFormat();
    swapchainCI.imageColorSpace          = m_surface->GetColorSpace();
    swapchainCI.imageExtent              = { width, height };
    swapchainCI.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform             = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers         = 1;
    swapchainCI.queueFamilyIndexCount    = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount    = 0;
    swapchainCI.pQueueFamilyIndices      = NULL;
    swapchainCI.presentMode              = m_presentMode;
    swapchainCI.oldSwapchain             = oldSwapchain;
    swapchainCI.clipped                  = true;
    swapchainCI.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    Tools::VkDebug::Graph("Swapchain::ReSetup() : create swapchain struct...");

    //PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(m_instance, "vkGetDeviceProcAddr");
    //PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(*m_device, "vkCreateSwapchainKHR");

    if (vkCreateSwapchainKHR(*m_device, &swapchainCI, nullptr, &m_swapchain) != VK_SUCCESS) {
        Tools::VkDebug::Error("Swapchain::ReSetup() : failed to create swapchain!");
        return false;
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    //! Note: destroying the swapchain also cleans up all its associated
    //! presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(*m_device, oldSwapchain, nullptr);

    Tools::VkDebug::Graph("Swapchain::ReSetup() : swapchain successfully re-configured!");

    return true;
}

void EvoVulkan::Types::Swapchain::Free() {
    Tools::VkDebug::Log("Swapchain::Free() : free swapchain pointer...");
    delete this;
}

void EvoVulkan::Types::Swapchain::Destroy() {
    Tools::VkDebug::Log("Swapchain::Destroy() : destroy vulkan swapchain...");

    if (!IsReady()) {
        Tools::VkDebug::Error("Swapchain::Destroy() : swapchain isn't ready!");
        return;
    }

    vkDestroySwapchainKHR(*m_device, m_swapchain, nullptr);
    this->m_swapchain = VK_NULL_HANDLE;

    this->m_device      = nullptr;
    this->m_surface     = nullptr;
    this->m_presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    this->m_instance    = VK_NULL_HANDLE;
}

bool EvoVulkan::Types::Swapchain::IsReady() const noexcept {
    return  m_swapchain != VK_NULL_HANDLE &&
            m_device != nullptr &&
            m_instance != VK_NULL_HANDLE &&
            m_surface != nullptr &&
            m_presentMode != VK_PRESENT_MODE_MAX_ENUM_KHR;
}
