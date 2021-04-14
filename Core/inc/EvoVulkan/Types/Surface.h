//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SURFACE_H
#define EVOVULKAN_SURFACE_H

#include <vulkan/vulkan.h>

namespace EvoVulkan::Types {
    class Surface {
    public:
        Surface(const Surface&) = delete;
    private:
        Surface() = default;
        ~Surface() = default;
    private:
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    public:
        operator VkSurfaceKHR() const { return m_surface; }

        static Surface* Create(const VkSurfaceKHR& surfaceKhr) {
            auto* surface = new Surface();
            surface->m_surface = surfaceKhr;

            return surface;
        }
    };
}

#endif //EVOVULKAN_SURFACE_H
