//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SWAPCHAIN_H
#define EVOVULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

namespace EvoVulkan::Types {
    class Device;
    class Surface;

    class Swapchain {
    public:
        Swapchain(const Swapchain&) = delete;
    private:
        VkSwapchainKHR   m_swapchain       = VK_NULL_HANDLE;
        Device*          m_device          = nullptr;
        Surface*         m_surface         = nullptr;
        VkInstance       m_instance        = VK_NULL_HANDLE;

        VkPresentModeKHR m_presentMode     = VK_PRESENT_MODE_MAX_ENUM_KHR;
    private:
        Swapchain() = default;
        ~Swapchain() = default;
    public:
        [[nodiscard]] bool IsReady() const noexcept;

        bool ReSetup(
            unsigned int width,
            unsigned int height);
    public:
        static Swapchain* Create(
                const VkInstance& instance,
                Surface* surface,
                Device* device,
                unsigned int width,
                unsigned int height);

        void Destroy();

        void Free();
    };
}

#endif //EVOVULKAN_SWAPCHAIN_H
