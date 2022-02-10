//
// Created by Monika on 07.02.2022.
//

#ifndef EVOVULKAN_INSTANCE_H
#define EVOVULKAN_INSTANCE_H

namespace EvoVulkan::Types {
    class Instance {
    private:
        explicit Instance(uint32_t version)
            : m_instance(VK_NULL_HANDLE)
            , m_version(version)
        { }

        Instance()
            : Instance(UINT32_MAX)
        { }

    public:
        static Instance* Create(
                const std::string& appName, const std::string& engineName,
                std::vector<const char*> extensions,
                const std::vector<const char*>& layers, const bool& validationEnabled)
        {
            Tools::VkDebug::Graph("Instance::Create() : create vulkan instance...");

            static bool exists = false;

            if (exists) {
                Tools::VkDebug::Error("Instance::Create() : instance already exists!");
                return nullptr;
            } else
                exists = true;

            if (extensions.empty()) {
                Tools::VkDebug::Error("Instance::Create() : extensions is empty!");
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

                if (layers.empty()) {
                    Tools::VkDebug::Error("Instance::Create() : layers is empty!");
                    return nullptr;
                }

                instInfo.enabledLayerCount   = (uint32_t)layers.size();
                instInfo.ppEnabledLayerNames = layers.data();

                Tools::PopulateDebugMessengerCreateInfo(debugCreateInfo);
                instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
            } else {
                instInfo.enabledLayerCount = 0;
                instInfo.pNext = nullptr;
            }

            VkResult result = vkCreateInstance(&instInfo, NULL, &instance->m_instance);
            if (result != VK_SUCCESS) {
                VK_ERROR("Instance::Create() : failed create vulkan instance! Reason: " + Tools::Convert::result_to_description(result));
                return nullptr;
            }

            Tools::VkDebug::Graph("Instance::Create() : instance successfully created!");

            return instance;
        }

    public:
        [[nodiscard]] uint32_t GetVersion() const {
            return m_version;
        }

        [[nodiscard]] bool Valid() const {
            return m_instance != VK_NULL_HANDLE;
        }

        void SetInstance(VkInstance instance) {
            m_instance = instance;
        }

        void Destroy() {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }

        void Free() {
            delete this;
        }

        operator VkInstance() const { return m_instance; }

    private:
        VkInstance m_instance;
        uint32_t m_version;

    };
}

#endif //EVOVULKAN_INSTANCE_H
