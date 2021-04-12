//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANTOOLS_H
#define EVOVULKAN_VULKANTOOLS_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace EvoVulkan::Tools {
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
            VkDebugReportFlagsEXT       flags,
            VkDebugReportObjectTypeEXT  objectType,
            uint64_t                    object,
            size_t                      location,
            int32_t                     messageCode,
            const char*                 pLayerPrefix,
            const char*                 pMessage,
            void*                       pUserData)
    {
        printf("VkDebugReportCallback: %s\n", pMessage);
        return VK_FALSE;    // Т.к. мы не хотим чтобы вызывающая функция упала.
    }

    static VkInstance CreateInstance(
            const std::string& appName, const std::string& engineName,
            const std::vector<const char*>& extensions,
            const std::vector<const char*>& layers, const bool& debug)
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName  = appName.c_str();
        appInfo.engineVersion     = 1;
        appInfo.apiVersion        = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        if (debug) {
            instInfo.enabledLayerCount   = layers.size();
            instInfo.ppEnabledLayerNames = layers.data();
        }

        instInfo.enabledExtensionCount   = extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.data();

        VkInstance instance = VK_NULL_HANDLE;
        if (vkCreateInstance(&instInfo, NULL, &instance) != VK_SUCCESS) {
            VK_DEBUG_ERROR("VulkanTools::CreateInstance() : failed create vulkan instance!");
            return VK_NULL_HANDLE;
        }

        if (debug) {
            // Получаем адрес функции vkCreateDebugReportCallbackEXT
            auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(
                    instance, "vkCreateDebugReportCallbackEXT"));

            // Регистрируем функцию отладки
            VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
            callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            callbackCreateInfo.pNext = NULL;
            callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                       VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                       VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            callbackCreateInfo.pfnCallback = &DebugReportCallback;
            callbackCreateInfo.pUserData = NULL;

            VkDebugReportCallbackEXT callback = VK_NULL_HANDLE;
            if (vkCreateDebugReportCallbackEXT(instance, &callbackCreateInfo, NULL, &callback) != VK_SUCCESS) {
                VK_DEBUG_ERROR("VulkanTools::CreateInstance() : failed create debug report callback!");
                return VK_NULL_HANDLE;
            }
        }

        return instance;
    }
}

#endif //EVOVULKAN_VULKANTOOLS_H
