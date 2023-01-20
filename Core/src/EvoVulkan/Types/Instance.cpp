//
// Created by Monika on 26.05.2022.
//

#include <EvoVulkan/Types/Instance.h>
#include <EvoVulkan/Tools/StringUtils.h>

namespace EvoVulkan::Types {
    Instance::~Instance() {
        if (m_instance) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    Instance* Instance::Create(
            const std::string& appName,
            const std::string& engineName,
            StringVector extensions,
            const StringVector& layers,
            bool validationEnabled
    ) {
        const auto&& logExtensions = Tools::Combine<const char*>(extensions, [](const char* str, int32_t i, bool last) -> std::string {
            return last ? std::string(str) : std::string(str).append(", ");
        });

        const auto&& logLayers = Tools::Combine<const char*>(layers, [](const char* str, int32_t i, bool last) -> std::string {
            return last ? std::string(str) : std::string(str).append(", ");
        });

        const auto log = Tools::Format("\n\tApplication name: %s\n\tEngine name: %s\n\tValidation enabled: %s\n\tExtensions: %s\n\tLayers: %s",
            appName.c_str(), engineName.c_str(), validationEnabled ? "true" : "false", logExtensions.c_str(), logLayers.c_str()
        );

        VK_GRAPH("Instance::Create() : create vulkan instance..." + log);

        static bool exists = false;

        if (exists) {
            VK_ERROR("Instance::Create() : instance already exists!");
            return nullptr;
        }
        else
            exists = true;

        if (extensions.empty()) {
            VK_ERROR("Instance::Create() : extensions is empty!");
            return nullptr;
        }

        if (validationEnabled)
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        auto* instance = new Instance(VK_API_VERSION_1_2);

        VkApplicationInfo appInfo  = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.engineVersion      = 1;
        appInfo.pEngineName        = engineName.c_str();
        //appInfo.apiVersion         = VK_API_VERSION_1_0;
        appInfo.apiVersion         = instance->m_version;//VK_MAKE_VERSION(1, 0, 2);

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        instInfo.enabledExtensionCount   = (uint32_t)extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.data();

        if (validationEnabled) {
            static VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            debugCreateInfo.pNext = nullptr;

            if (layers.empty()) {
                VK_ERROR("Instance::Create() : layers is empty!");
                return nullptr;
            }

            instInfo.enabledLayerCount   = (uint32_t)layers.size();
            instInfo.ppEnabledLayerNames = layers.data();

            Tools::PopulateDebugMessengerCreateInfo(debugCreateInfo);
            instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }
        else {
            instInfo.enabledLayerCount = 0;
            instInfo.pNext = nullptr;
        }

        VkResult result = vkCreateInstance(&instInfo, NULL, &instance->m_instance);
        if (result != VK_SUCCESS) {
            VK_ERROR("Instance::Create() : failed create vulkan instance! Reason: " + Tools::Convert::result_to_description(result));
            return nullptr;
        }

        VK_GRAPH("Instance::Create() : instance successfully created!");

        return instance;
    }

    uint32_t Instance::GetVersion() const {
        return m_version;
    }
    
    bool Instance::IsReady() const {
        return m_instance != VK_NULL_HANDLE;
    }
}