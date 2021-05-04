//
// Created by Nikita on 04.05.2021.
//

#ifndef EVOVULKAN_DEPTHSTENCIL_H
#define EVOVULKAN_DEPTHSTENCIL_H

#include <vulkan/vulkan.h>
#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Device;
    class Swapchain;

    class DepthStencil : public IVkObject {
    public:
        DepthStencil(const DepthStencil&) = delete;
    private:
        DepthStencil()  = default;
        ~DepthStencil() = default;
    private:
        VkImage          m_image     = VK_NULL_HANDLE;
        VkDeviceMemory   m_mem       = VK_NULL_HANDLE;
        VkImageView      m_view      = VK_NULL_HANDLE;

        const Device*    m_device    = nullptr;
        const Swapchain* m_swapchain = nullptr;
    public:
        bool ReCreate(uint32_t width, uint32_t height);
        [[nodiscard]] bool IsReady() const override;

        [[nodiscard]] inline VkImageView GetImageView() const noexcept { return m_view; }

        void Destroy() override;
        void Free()    override;
    public:
        static DepthStencil* Create(const Device* device, const Swapchain* swapchain, uint32_t width, uint32_t height);
    };
}

#endif //EVOVULKAN_DEPTHSTENCIL_H
