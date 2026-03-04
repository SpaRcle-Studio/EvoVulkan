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
            std::set<std::string> supportedExtensions,
            StringVector extensions,
            const StringVector& layers,
            bool gpuAssistEnabled,
            bool validationLayersEnabled,
            bool validationReportEnabled
    ) {
        auto&& pInstance = new Instance(VK_API_VERSION_1_2);
        pInstance->m_supportedExtensions = std::move(supportedExtensions);

        /// enable VK_KHR_portability_enumeration if it is supported
        if (pInstance->IsExtensionSupported(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 1) {
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        }

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
            VK_LOG("Instance::Create() : extensions are empty");
        }

        VkApplicationInfo appInfo  = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion      = 1;
        appInfo.pEngineName        = engineName.c_str();
        //appInfo.apiVersion         = VK_API_VERSION_1_0;
        appInfo.apiVersion         = pInstance->m_version;//VK_MAKE_VERSION(1, 0, 2);

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        if (pInstance->IsExtensionSupported(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 1) {
            VK_LOG("Instance::Create() : VK_KHR_portability_enumeration extension is supported, enabling it...");
            instInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }

        instInfo.enabledExtensionCount   = (uint32_t)extensions.size();
        instInfo.ppEnabledExtensionNames = extensions.data();

        for (const auto& ext : extensions) {
            pInstance->m_enabledExtensions.insert(ext);
        }

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

                pInstance->m_validationEnabled = true;

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

        VkResult result = vkCreateInstance(&instInfo, NULL, &pInstance->m_instance);
        if (result != VK_SUCCESS) {
            VK_ERROR("Instance::Create() : failed create vulkan instance! Reason: " + Tools::Convert::result_to_description(result));
            return nullptr;
        }

        VK_GRAPH("Instance::Create() : instance is created successfully!");

        return pInstance;
    }

    uint32_t Instance::GetVersion() const {
        return m_version;
    }
    
    bool Instance::IsReady() const {
        return m_instance != VK_NULL_HANDLE;
    }

    bool Instance::IsExtensionSupported(const std::string_view& extension) const {
        return m_supportedExtensions.count(extension.data()) == 1;
    }

    bool Instance::IsExtensionEnabled(const std::string_view& extension) const {
        return m_enabledExtensions.count(extension.data()) == 1;
    }
}