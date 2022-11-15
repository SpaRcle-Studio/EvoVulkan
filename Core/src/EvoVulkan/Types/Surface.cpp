//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/Types/Surface.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Types {
    Surface* Surface::Create(VkSurfaceKHR const &surfaceKhr, VkInstance const &instance, void *windowHandle) {
        auto&& surface = new Surface();

        surface->m_surface  = surfaceKhr;
        surface->m_instance = instance;
        surface->m_windowHandle = windowHandle;

        return surface;
    }

    bool Surface::Init(const EvoVulkan::Types::Device *device) {
        VK_GRAPH("Surface::Init() : initialize vulkan surface...");

        /// Get list of supported formats
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(*device, this->m_surface, &m_countSurfaceFormats, NULL) != VK_SUCCESS) {
            VK_ERROR("Surface::Init() : failed get physical device surface format!");
            return false;
        }

        if (m_countSurfaceFormats == 0) {
            VK_ERROR("Surface::Init : count surface formats is zero!");
            return false;
        }

        m_surfFormats = (VkSurfaceFormatKHR *)malloc(m_countSurfaceFormats * sizeof(VkSurfaceFormatKHR));
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(*device, this->m_surface, &m_countSurfaceFormats, this->m_surfFormats) != VK_SUCCESS) {
            VK_ERROR("Surface::Init() : failed get physical device surface format!");
            return false;
        }

        m_isInit = true;

        return true;
    }

    void EvoVulkan::Types::Surface::Destroy() {
        VK_LOG("Surface::Destroy() : destroy vulkan surface...");

        if (!Ready()) {
            VK_ERROR("Surface::Destroy() : surface isn't ready!");
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
}
