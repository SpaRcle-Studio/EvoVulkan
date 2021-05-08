//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANTOOLS_H
#define EVOVULKAN_VULKANTOOLS_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Types/CmdBuffer.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/DepthStencil.h>
#include <EvoVulkan/Types/Synchronization.h>
#include <EvoVulkan/Types/Pipeline.h>

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanHelper.h>

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <array>

#include <EvoVulkan/Tools/VulkanConverter.h>

#include <functional>
#include <fstream>

namespace EvoVulkan::Tools {
    VkShaderModule LoadShaderModule(const char *fileName, VkDevice device);

    VkPipelineLayout CreatePipelineLayout(const VkDevice& device, VkDescriptorSetLayout descriptorSetLayout);

    static VkDescriptorSetLayout CreateDescriptorLayout(
            const VkDevice& device,
            const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings)
    {
        auto descriptorSetLayoutCreateInfo = Initializers::DescriptorSetLayoutCreateInfo(
                setLayoutBindings.data(),
                static_cast<uint32_t>(setLayoutBindings.size()));

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        auto result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateDescriptorLayout() : failed to create descriptor set layout!");
            return VK_NULL_HANDLE;
        }
        else
            return descriptorSetLayout;
    }

    Types::Pipeline* CreateStandardGeometryPipeLine(
            const Types::Device* device,
            const std::vector<VkDynamicState>& dynamicStateEnables,
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
            VkVertexInputBindingDescription vertexInputBinding,
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
            VkPipelineCache pipelineCache);

    static VkCommandBuffer* AllocateCommandBuffers(const VkDevice& device, VkCommandBufferAllocateInfo allocInfo) {
        auto cmdBuffs = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * allocInfo.commandBufferCount);

        auto result = vkAllocateCommandBuffers(device, &allocInfo, cmdBuffs);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::AllocateCommandBuffers() : failed to allocate vulkan command buffer!");
            return nullptr;
        }

        return cmdBuffs;
    }

    static void FreeCommandBuffers(const VkDevice& device, const VkCommandPool& cmdPool, VkCommandBuffer** cmdBuffs, uint32_t count) {
        if (cmdBuffs && *cmdBuffs) {
            vkFreeCommandBuffers(device, cmdPool, count, *cmdBuffs);

            free(*cmdBuffs);
            *cmdBuffs = nullptr;
        } else
            VK_ERROR("Tools::FreeCommandBuffers() : command buffers in nullptr!");
    }

    void DestroyPipelineCache(const VkDevice& device, VkPipelineCache* cache);

    VkPipelineCache CreatePipelineCache(const VkDevice& device);

    void DestroySynchronization(const VkDevice& device, Types::Synchronization* sync);

    Types::Synchronization CreateSynchronization(const VkDevice& device);

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static VkDebugUtilsMessengerEXT SetupDebugMessenger(const VkInstance& instance) {
        Tools::VkDebug::Graph("VulkanTools::SetupDebugMessenger() : setup vulkan debug messenger...");

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        Tools::PopulateDebugMessengerCreateInfo(createInfo);

        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        auto result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS) {
            Tools::VkDebug::Error("VulkanTools::SetupDebugMessenger() : failed to set up debug messenger! Reason: "
                + Tools::Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return debugMessenger;
    }

    static VkInstance CreateInstance(
            const std::string& appName, const std::string& engineName,
            std::vector<const char*> extensions,
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

        if (validationEnabled)
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        VkApplicationInfo appInfo  = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.engineVersion      = 1;
        appInfo.pEngineName        = engineName.c_str();
        //appInfo.apiVersion         = VK_API_VERSION_1_0;
        appInfo.apiVersion         = VK_API_VERSION_1_2;//VK_MAKE_VERSION(1, 0, 2);

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        instInfo.enabledExtensionCount   = (uint32_t)extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.data();

        if (validationEnabled) {
            static VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

            if (layers.empty()) {
                Tools::VkDebug::Error("VulkanTools::CreateInstance() : layers is empty!");
                return VK_NULL_HANDLE;
            }

            instInfo.enabledLayerCount   = (uint32_t)layers.size();
            instInfo.ppEnabledLayerNames = layers.data();

            Tools::PopulateDebugMessengerCreateInfo(debugCreateInfo);
            instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            instInfo.enabledLayerCount = 0;
            instInfo.pNext = nullptr;
        }

        VkInstance instance = VK_NULL_HANDLE;
        VkResult result = vkCreateInstance(&instInfo, NULL, &instance);
        if (result != VK_SUCCESS) {
            VkDebug::Error("VulkanTools::CreateInstance() : failed create vulkan instance! Reason: " + Tools::Convert::result_to_description(result));
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
        if (surfaceKhr == VK_NULL_HANDLE) {
            Tools::VkDebug::Error("VulkanTools::CreateSurface() : failed platform-create vulkan surface!");
            return nullptr;
        }

        Types::Surface* surface = Types::Surface::Create(
                surfaceKhr,
                instance);

        return surface;
    }

    static VkDevice CreateLogicalDevice(
            VkPhysicalDevice physicalDevice,
            Types::FamilyQueues *pQueues,
            const std::vector<const char *> &extensions,
            const std::vector<const char *> &validLayers,
            VkPhysicalDeviceFeatures deviceFeatures)
    {
        Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : create vulkan logical device...");

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { pQueues->GetGraphicsIndex(), pQueues->GetPresentIndex() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        //!=============================================================================================================

        VkDeviceCreateInfo createInfo      = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();

        createInfo.pEnabledFeatures        = &deviceFeatures;

        createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (validLayers.empty()) {
            Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : validation layers disabled.");
            createInfo.enabledLayerCount = 0;
        } else {
            Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : validation layers enabled.");

            createInfo.enabledLayerCount   = static_cast<uint32_t>(validLayers.size());
            createInfo.ppEnabledLayerNames = validLayers.data();
        }

        VkDevice device = VK_NULL_HANDLE;
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : failed to create logical device!");
            return VK_NULL_HANDLE;
        }

        return device;
    }

    static Types::Device* CreateDevice(
            const VkInstance& instance, const Types::Surface* surface,
            const std::vector<const char*>& extensions,
            const std::vector<const char*>& validationLayers,
            const bool& enableSampleShading)
    {
        Tools::VkDebug::Graph("VulkanTools::CreateDevice() : create vulkan device...");

        Types::FamilyQueues* queues         = nullptr;

        VkPhysicalDevice     physicalDevice = VK_NULL_HANDLE;
        VkDevice             logicalDevice  = VK_NULL_HANDLE;

        //!=============================================================================================================

        auto devices = Tools::GetAllDevices(instance);
        if (devices.empty()) {
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : not found device with vulkan support!");
            return nullptr;
        }

        for (const auto& device : devices)
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : found device - " + Types::GetDeviceName(device));

        for (auto physDev : devices) {
            if (Types::IsDeviceSuitable(physDev, (*surface), extensions)) {
                if (physicalDevice == VK_NULL_HANDLE) {
                    physicalDevice = physDev;
                    continue;
                }

                if (Types::Device::IsBetterThan(physDev, physicalDevice))
                    physicalDevice = physDev;
            } else
                Tools::VkDebug::Warn("VulkanTools::CreateDevice() : device \"" +
                    Types::GetDeviceName(physDev) + "\" isn't suitable!");
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            std::string msg = std::string();

            for (auto extension : extensions) {
                msg += "\n\t";
                msg += extension;
            }

            Tools::VkDebug::Error("VulkanTools::CreateDevice() : not found suitable device! \nExtensions: " + msg);

            return nullptr;
        } else
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : select \""
                + Types::GetDeviceName(physicalDevice) + "\" device.");

        queues = Types::FamilyQueues::Find(physicalDevice, surface);
        if (!queues->IsComplete()) {
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : family queues isn't complete!");
            return nullptr;
        }

        //!=============================================================================================================

        VkPhysicalDeviceFeatures deviceFeatures = {};

        logicalDevice = Tools::CreateLogicalDevice(
                physicalDevice,
                queues,
                extensions,
                validationLayers,
                deviceFeatures);

        if (logicalDevice == VK_NULL_HANDLE) {
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : failed create logical device!");
            return nullptr;
        }

        Tools::VkDebug::Graph("VulkanTools::CreateDevice() : set logical device queues...");
        {
            VkQueue graphics = VK_NULL_HANDLE;
            //VkQueue present  = VK_NULL_HANDLE;

            vkGetDeviceQueue(logicalDevice, queues->GetGraphicsIndex(), 0, &graphics);
            //vkGetDeviceQueue(logicalDevice, queues->GetPresentIndex(), 0, &present);

            //queues->SetQueues(graphics, present);
            queues->SetQueue(graphics);
        }

        auto finallyDevice = Types::Device::Create(physicalDevice, logicalDevice, queues, enableSampleShading);

        if (finallyDevice)
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : device successfully created!");
        else
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : failed to create device!");

        return finallyDevice;
    }

    static void DestroyRenderPass(const Types::Device* device, VkRenderPass* renderPass) {
        VK_LOG("Tools::DestroyRenderPass() : destroy vulkan render pass...");

        if (renderPass && *renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(*device, *renderPass, nullptr);
            *renderPass = VK_NULL_HANDLE;
        } else
            VK_ERROR("Tools::DestroyRenderPass() : render pass is nullptr!");
    }

    VkRenderPass CreateRenderPass(const Types::Device* device, const Types::Swapchain* swapchain);
}

#endif //EVOVULKAN_VULKANTOOLS_H
