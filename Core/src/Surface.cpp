//
// Created by Nikita on 12.04.2021.
//

#include "EvoVulkan/Types/Surface.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

#include <EvoVulkan/Types/Device.h>

bool EvoVulkan::Types::Surface::Init(const EvoVulkan::Types::Device *device) {
    Tools::VkDebug::Graph("Surface::Init() : initialize vulkan surface...");

    // Get list of supported formats
    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(*device, this->m_surface, &formatCount, NULL) != VK_SUCCESS) {
        Tools::VkDebug::Error("Surface::Init() : failed get physical device surface format!");
        return false;
    }

    this->m_surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(*device, this->m_surface, &formatCount, this->m_surfFormats) != VK_SUCCESS) {
        Tools::VkDebug::Error("Surface::Init() : failed get physical device surface format!");
        return false;
    }

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.

    if (formatCount == 0) {
        Tools::VkDebug::Error("Surface::Init() : count formats is zero!");
        return false;
    }

    if (formatCount == 1 && m_surfFormats[0].format == VK_FORMAT_UNDEFINED)
        m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    else
        m_colorFormat = m_surfFormats[0].format;

    m_colorSpace = m_surfFormats[0].colorSpace;

    if (m_colorFormat == VK_FORMAT_UNDEFINED) {
        Tools::VkDebug::Error("Surface::Init() : color format undefined!");
        return false;
    }

    this->m_isInit = true;

    return true;
}

void EvoVulkan::Types::Surface::Destroy() {
    Tools::VkDebug::Log("Surface::Destroy() : destroy vulkan surface...");

    if (!Ready()) {
        Tools::VkDebug::Error("Surface::Destroy() : surface isn't ready!");
        return;
    }

    free(m_surfFormats);
    m_surfFormats = nullptr;

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    m_surface     = VK_NULL_HANDLE;

    m_colorFormat = VK_FORMAT_UNDEFINED;
    m_colorSpace  = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

    m_instance    = VK_NULL_HANDLE;
}

void EvoVulkan::Types::Surface::Free() {
    delete this;
}

bool EvoVulkan::Types::Surface::Ready() const {
    return m_isInit && m_surface != VK_NULL_HANDLE && m_surfFormats;
}
