//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SURFACE_H
#define EVOVULKAN_SURFACE_H

#ifdef __MINGW32__
    #pragma GCC diagnostic ignored "-Wattributes"
#endif

#include <vulkan/vulkan.h>

namespace EvoVulkan::Types {
    class Device;

    class Surface {
    public:
        Surface(const Surface&) = delete;
    private:
        Surface() = default;
        ~Surface() = default;
    private:
        bool                m_isInit              = false;

        VkSurfaceKHR        m_surface             = VK_NULL_HANDLE;

        uint32_t            m_countSurfaceFormats = 0;
        VkSurfaceFormatKHR* m_surfFormats         = nullptr;

        VkInstance          m_instance            = VK_NULL_HANDLE;
        void*               m_windowHandle        = nullptr;

    public:
        operator VkSurfaceKHR() const { return m_surface; }

        static Surface* Create(const VkSurfaceKHR& surfaceKhr, const VkInstance& instance, void* windowHandle) {
            auto* surface = new Surface();

            surface->m_surface  = surfaceKhr;
            surface->m_instance = instance;
            surface->m_windowHandle = windowHandle;

            return surface;
        }
    public:
        [[nodiscard]] bool IsInit() const noexcept { return m_isInit; }
        bool Init(const Types::Device* device);

        [[nodiscard]] bool Ready() const;

        [[nodiscard]] VkSurfaceFormatKHR* GetSurfaceFormats() const { return m_surfFormats;         }
        [[nodiscard]] uint32_t            GetCountSurfFmts()  const { return m_countSurfaceFormats; }
        [[nodiscard]] void*               GetHandle()         const { return m_windowHandle; }

        void Destroy();
        void Free();
    };
}

#endif //EVOVULKAN_SURFACE_H
