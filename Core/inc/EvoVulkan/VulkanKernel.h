//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANKERNEL_H
#define EVOVULKAN_VULKANKERNEL_H

/*
    1. helper
    2. types
    3. tools
    4. core
 */

#include <EvoVulkan/Tools/VulkanTools.h>

namespace EvoVulkan::Core {
    class VulkanKernel {
    public:
        VulkanKernel(const VulkanKernel&) = delete;
    private:
        VulkanKernel()  = default;
        ~VulkanKernel() = default;
    private:
        std::vector<const char*> m_instExtensions    = {};
        std::vector<const char*> m_validationLayers  = {};

        std::string              m_appName           = "Unknown";
        std::string              m_engineName        = "NoEngine";

        VkInstance               m_instance          = VK_NULL_HANDLE;
        Types::Device*           m_device            = nullptr;
        Types::Surface*          m_surface           = nullptr;
        Types::Swapchain*        m_swapchain         = nullptr;

        VkDebugUtilsMessengerEXT m_debugMessenger    = VK_NULL_HANDLE;

        bool                     m_validationEnabled = false;

        bool                     m_isPreInitialized  = false;
        bool                     m_isInitialized     = false;
        bool                     m_isPostInitialized = false;

        unsigned int             m_width             = 0;
        unsigned int             m_height            = 0;
    public:
        inline bool SetValidationLayersEnabled(const bool& value) {
            if (m_isPreInitialized) {
                Tools::VkDebug::Error("VulkanKernel::SetValidationLayersEnabled() : at this stage it is not possible to set this parameter!");
                return false;
            }

            this->m_validationEnabled = value;

            return true;
        }
        inline void SetSize(unsigned int width, unsigned int height) {
            this->m_width  = width;
            this->m_height = height;
        }
    public:
        static VulkanKernel* Create();
        bool Free();
    public:
        bool PreInit(
                const std::string& appName,
                const std::string& engineName,
                const std::vector<const char*>& instExtensions,
                const std::vector<const char*>& validationLayers
                );

        bool Init(
                const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
                const std::vector<const char*>& deviceExtensions, const bool& enableSampleShading);
        bool PostInit();
    };
}

#endif //EVOVULKAN_VULKANKERNEL_H
