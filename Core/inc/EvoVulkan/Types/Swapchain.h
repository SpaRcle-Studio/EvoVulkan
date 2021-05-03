//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SWAPCHAIN_H
#define EVOVULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Device;
    class Surface;
    class CmdBuffer;

    typedef struct _SwapChainBuffers {
        VkImage     m_image;
        VkImageView m_view;
    } SwapChainBuffer;

    class Swapchain : public IVkObject {
    public:
        Swapchain(const Swapchain&) = delete;
    private:
        VkSwapchainKHR   m_swapchain       = VK_NULL_HANDLE;
        Device*          m_device          = nullptr;
        Surface*         m_surface         = nullptr;
        VkInstance       m_instance        = VK_NULL_HANDLE;

        VkPresentModeKHR m_presentMode     = VK_PRESENT_MODE_MAX_ENUM_KHR;

        VkFormat         m_depthFormat     = {};
        VkFormat         m_colorFormat     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR  m_colorSpace      = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

        //! note: images will be automatic destroyed after destroying swapchain
        VkImage*         m_swapchainImages = nullptr;
        uint32_t         m_countImages     = 0;

        SwapChainBuffer* m_buffers         = nullptr;
    private:
        bool InitFormats();

        bool CreateBuffers();
        void DestroyBuffers();

        bool CreateImages();
    private:
        Swapchain()  = default;
        ~Swapchain() = default;
    public:
        [[nodiscard]] bool IsReady() const override;

        bool ReSetup(
            unsigned int width,
            unsigned int height,
            const CmdBuffer* cmdBuff);

        [[nodiscard]] VkFormat GetDepthFormat()       const { return m_depthFormat; }
        [[nodiscard]] VkFormat GetColorFormat()       const { return m_colorFormat; }
        [[nodiscard]] VkColorSpaceKHR GetColorSpace() const { return m_colorSpace;  }
    public:
        static Swapchain* Create(
                const VkInstance& instance,
                Surface* surface,
                Device* device,
                const CmdBuffer* cmdBuff,
                unsigned int width,
                unsigned int height);

        void Destroy() override;
        void Free() override;
    };
}

#endif //EVOVULKAN_SWAPCHAIN_H
