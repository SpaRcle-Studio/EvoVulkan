//
// Created by Nikita on 12.04.2021.
//

#include "EvoVulkan/Types/Surface.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

#include <EvoVulkan/Types/Device.h>

bool EvoVulkan::Types::Surface::Init(const EvoVulkan::Types::Device *device) {
    Tools::VkDebug::Graph("Surface::Init() : initialize vulkan surface...");

    // Get list of supported formats
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(*device, this->m_surface, &m_countSurfaceFormats, NULL) != VK_SUCCESS) {
        Tools::VkDebug::Error("Surface::Init() : failed get physical device surface format!");
        return false;
    }

    if (m_countSurfaceFormats == 0) {
        Tools::VkDebug::Error("Surface::Init : count surface formats is zero!");
        return false;
    }

    this->m_surfFormats = (VkSurfaceFormatKHR *)malloc(m_countSurfaceFormats * sizeof(VkSurfaceFormatKHR));
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(*device, this->m_surface, &m_countSurfaceFormats, this->m_surfFormats) != VK_SUCCESS) {
        Tools::VkDebug::Error("Surface::Init() : failed get physical device surface format!");
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

    m_instance    = VK_NULL_HANDLE;
}

void EvoVulkan::Types::Surface::Free() {
    delete this;
}

bool EvoVulkan::Types::Surface::Ready() const {
    return m_isInit && m_surface != VK_NULL_HANDLE && m_surfFormats;
}
