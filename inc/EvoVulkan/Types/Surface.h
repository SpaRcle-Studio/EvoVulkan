//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SURFACE_H
#define EVOVULKAN_SURFACE_H

#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Device;

    class DLL_EVK_EXPORT Surface : public IVkObject {
    private:
        Surface() = default;

    public:
        ~Surface() override;

        static Surface* Create(const VkSurfaceKHR& surfaceKhr, const VkInstance& instance, void* windowHandle);

        operator VkSurfaceKHR() const { return m_surface; }

    public:
        bool Init(const Types::Device* device);

        EVK_NODISCARD bool IsReady() const override;
        EVK_NODISCARD bool IsInit() const noexcept { return m_isInit; }
        EVK_NODISCARD VkSurfaceFormatKHR* GetSurfaceFormats() const { return m_surfFormats; }
        EVK_NODISCARD uint32_t GetCountSurfFmts()  const { return m_countSurfaceFormats; }
        EVK_NODISCARD void* GetHandle() const { return m_windowHandle; }

    private:
        bool                m_isInit              = false;
        uint32_t            m_countSurfaceFormats = 0;

        VkSurfaceKHR        m_surface             = VK_NULL_HANDLE;
        VkInstance          m_instance            = VK_NULL_HANDLE;

        VkSurfaceFormatKHR* m_surfFormats         = nullptr;
        void*               m_windowHandle        = nullptr;

    };
}

#endif //EVOVULKAN_SURFACE_H
