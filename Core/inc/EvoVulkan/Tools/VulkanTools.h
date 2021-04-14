//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANTOOLS_H
#define EVOVULKAN_VULKANTOOLS_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>

#include <EvoVulkan/Tools/VulkanHelper.h>

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include <functional>

namespace EvoVulkan::Tools {
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static VkInstance CreateInstance(
            const std::string& appName, const std::string& engineName,
            const std::vector<const char*>& extensions,
            const std::vector<const char*>& layers, const bool& validationEnabled)
    {
        Tools::VkDebug::Graph("VulkanTools::CreateInstance() : create vulkan instance...");

        static bool exists = false;

        if (exists) {
            Tools::VkDebug::Error("VulkanTools::CreateInstance() : instance already exists!");
            return VK_NULL_HANDLE;
        } else
            exists = true;

        if (extensions.empty()) {
            Tools::VkDebug::Error("VulkanTools::CreateInstance() : extensions is empty!");
            return VK_NULL_HANDLE;
        }

        VkApplicationInfo appInfo = {};
        appInfo.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName  = appName.c_str();
        appInfo.engineVersion     = 1;
        appInfo.apiVersion        = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        instInfo.enabledExtensionCount   = extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.data();

        if (validationEnabled) {
            static VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

            if (layers.empty()) {
                Tools::VkDebug::Error("VulkanTools::CreateInstance() : layers is empty!");
                return VK_NULL_HANDLE;
            }

            instInfo.enabledLayerCount   = layers.size();
            instInfo.ppEnabledLayerNames = layers.data();

            Tools::PopulateDebugMessengerCreateInfo(debugCreateInfo);
            instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            instInfo.enabledLayerCount = 0;
            instInfo.pNext = nullptr;
        }

        VkInstance instance = VK_NULL_HANDLE;
        if (vkCreateInstance(&instInfo, NULL, &instance) != VK_SUCCESS) {
            VkDebug::Error("VulkanTools::CreateInstance() : failed create vulkan instance!");
            return VK_NULL_HANDLE;
        }

        Tools::VkDebug::Graph("VulkanTools::CreateInstance() : instance successfully created!");

        return instance;
    }
    static bool DestroyInstance(VkInstance* instance) {
        vkDestroyInstance(*instance, nullptr);
        *instance = VK_NULL_HANDLE;
        return true;
    }

    static Types::Surface* CreateSurface(const VkInstance& instance, const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate) {
        VkSurfaceKHR surfaceKhr = platformCreate(instance);

        Types::Surface* surface = Types::Surface::Create(surfaceKhr);

        return surface;
    }

    static Types::Device* CreateDevice(const VkInstance& instance, const Types::Surface* surface, const std::vector<const char*>& extensions) {
        Tools::VkDebug::Graph("VulkanTools::CreateDevice() : create vulkan logical device...");

        auto devices = Tools::GetAllDevices(instance);
        for (const auto& device : devices)
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : found device - " + Types::GetDeviceName(device));

        Types::Device* device = nullptr;
        Types::FamilyQueues* queues = nullptr;

        for (auto physDev : devices) {
            if (Types::IsDeviceSuitable(physDev, (*surface), extensions)) {
                //queues = findQueueFamilies(physicalDevice);
                //if (!queues->IsComplete())

                //device = Types::Device::Create(instance);
            }
        }

        return nullptr;
    }
}

#endif //EVOVULKAN_VULKANTOOLS_H
