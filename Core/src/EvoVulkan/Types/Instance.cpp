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
            bool gpuAssistEnabled,
            bool validationLayersEnabled,
            bool validationReportEnabled
    ) {
        const auto&& logExtensions = Tools::Combine<const char*>(extensions, [](const char* str, int32_t i, bool last) -> std::string {
            return last ? std::string(str) : std::string(str).append(", ");
        });

        const auto&& logLayers = Tools::Combine<const char*>(layers, [](const char* str, int32_t i, bool last) -> std::string {
            return last ? std::string(str) : std::string(str).append(", ");
        });

        const auto log = Tools::Format("\n\tApplication name: %s\n\tEngine name: %s\n\tValidation enabled: %s\n\tExtensions: %s\n\tLayers: %s",
            appName.c_str(), engineName.c_str(), validationLayersEnabled ? "true" : "false", logExtensions.c_str(), logLayers.c_str()
        );

        VK_GRAPH("Instance::Create() : creating vulkan instance..." + log);

        if (extensions.empty()) {
            VK_ERROR("Instance::Create() : extensions are empty!");
            return nullptr;
        }

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

        static VkValidationFeaturesEXT validationFeatures = {};
        static VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

        if (validationReportEnabled) {
            debugCreateInfo.pNext = nullptr;

            if (validationLayersEnabled) {
                if (layers.empty()) {
                    VK_ERROR("Instance::Create() : layers is empty!");
                    return nullptr;
                }

                instInfo.enabledLayerCount = (uint32_t) layers.size();
                instInfo.ppEnabledLayerNames = layers.data();
            }
            else {
                instInfo.enabledLayerCount = 0;
            }

            Tools::PopulateDebugMessengerCreateInfo(debugCreateInfo);

            static VkValidationFeatureEnableEXT enables[] = {
                VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
                VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
                VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT
            };

            if (validationLayersEnabled && gpuAssistEnabled) {
                VK_LOG("Instance::Create() : GPU Assisted Validation is enabled.");

                validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                validationFeatures.pNext = &debugCreateInfo;
                validationFeatures.enabledValidationFeatureCount = uint32_t(std::size(enables));
                validationFeatures.pEnabledValidationFeatures = enables;

                instInfo.pNext = &validationFeatures;
            }
            else {
                instInfo.pNext = &debugCreateInfo;
            }
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

        VK_GRAPH("Instance::Create() : instance is created successfully!");

        return instance;
    }

    uint32_t Instance::GetVersion() const {
        return m_version;
    }
    
    bool Instance::IsReady() const {
        return m_instance != VK_NULL_HANDLE;
    }
}