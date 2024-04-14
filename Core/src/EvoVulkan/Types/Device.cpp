//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/Types/Device.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Tools/VulkanConverter.h>
#include <EvoVulkan/Tools/DeviceTools.h>

namespace EvoVulkan::Types {
    Device::Device(Instance *pInstance, FamilyQueues* pQueues, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
        : m_instance(pInstance)
        , m_familyQueues(pQueues)
        , m_physicalDevice(physicalDevice)
        , m_logicalDevice(logicalDevice)
    { }

    Device::~Device() {
        VK_LOG("Device::Destroy() : destroying vulkan device...");

        if (m_familyQueues) {
            delete m_familyQueues;
            m_familyQueues = nullptr;
        }

        if (m_logicalDevice) {
            vkDestroyDevice(m_logicalDevice, nullptr);
            m_logicalDevice = VK_NULL_HANDLE;
        }
    }

    Device* Device::Create(EvoDeviceCreateInfo info) {
        VK_GRAPH("Device::Create() : create vulkan device...");

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice         logicalDevice  = VK_NULL_HANDLE;

        //!=============================================================================================================

        auto&& devices = Tools::GetAllDevices(*info.pInstance);
        if (devices.empty()) {
            VK_ERROR("Device::Create() : not found device with vulkan support!");
            return nullptr;
        }

        for (const auto& device : devices) {
            VK_LOG("Device::Create() : found device - " + Tools::GetDeviceName(device));
        }

        std::vector<Tools::DeviceSelectionInfo> deviceInfos;

        for (auto&& physDev : devices) {
            if (Tools::IsDeviceSuitable(physDev, info.pSurface ? *info.pSurface : nullptr, info.extensions)) {
                deviceInfos.emplace_back(Tools::GetSelectionDeviceInfo(physDev));
            }
            else {
                VK_WARN("Device::Create() : device \"" + Tools::GetDeviceName(physDev) + "\" isn't suitable!");
            }
        }

        physicalDevice = Tools::SelectBetterDevice(deviceInfos);

        if (!physicalDevice) {
            VK_ERROR("Device::Create() : suitable device is not found!");
            return nullptr;
        }

        if (Tools::IsExtensionSupported(physicalDevice, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) {
            info.extensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            info.extensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            info.extensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            std::string msg = std::string();

            for (auto extension : info.extensions) {
                msg += "\n\t";
                msg += extension;
            }

            VK_ERROR("Device::Create() : not found suitable device! \nExtensions: " + msg);

            return nullptr;
        }
        else {
            VK_LOG("Device::Create() : select \"" + Tools::GetDeviceName(physicalDevice) + "\" device.");
        }

        FamilyQueues* pQueues = FamilyQueues::Find(physicalDevice, info.pSurface);

        if (!pQueues) {
            VK_ERROR("Device::Create() : failed to find family queues!");
            return nullptr;
        }

        if (!pQueues->IsComplete()) {
            VK_ERROR("Device::Create() : family queues isn't complete!");
            delete pQueues;
            return nullptr;
        }

        //!=============================================================================================================

        std::string supportedExtMsg = "Device::Create() : supported device extensions:";
        auto&& supportedExtensions = Tools::GetSupportedDeviceExtensions(physicalDevice);
        for (auto&& extension : supportedExtensions) {
            supportedExtMsg.append("\n\t").append(extension);
        }
        VK_LOG(supportedExtMsg);

        VkPhysicalDeviceLineRasterizationFeaturesEXT lineRasterizationFeaturesExt = {};
        lineRasterizationFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
        lineRasterizationFeaturesExt.pNext = nullptr;

        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &lineRasterizationFeaturesExt;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

        VK_LOG(std::string("Device::Create() : VkPhysicalDeviceLineRasterizationFeaturesEXT: ")
           .append("\n\trectangularLines = ").append(lineRasterizationFeaturesExt.rectangularLines ? "True" : "False")
           .append("\n\tbresenhamLines = ").append(lineRasterizationFeaturesExt.bresenhamLines ? "True" : "False")
           .append("\n\tsmoothLines = ").append(lineRasterizationFeaturesExt.smoothLines ? "True" : "False")
           .append("\n\tstippledRectangularLines = ").append(lineRasterizationFeaturesExt.stippledRectangularLines ? "True" : "False")
           .append("\n\tstippledBresenhamLines = ").append(lineRasterizationFeaturesExt.stippledBresenhamLines ? "True" : "False")
           .append("\n\tstippledSmoothLines = ").append(lineRasterizationFeaturesExt.stippledSmoothLines ? "True" : "False")
        );

        //!=============================================================================================================

        //deviceFeatures.fillModeNonSolid  = true;
        //deviceFeatures.samplerAnisotropy = true;
        //deviceFeatures.textureCompressionBC       = true;
        ////deviceFeatures.textureCompressionETC2     = true;
        ////deviceFeatures.textureCompressionASTC_LDR = true;

        logicalDevice = Tools::CreateLogicalDevice(
                physicalDevice,
                pQueues,
                info.extensions,
                info.validationLayers);

        if (logicalDevice == VK_NULL_HANDLE) {
            VK_ERROR("Device::Create() : failed create logical device!");
            delete pQueues;
            return nullptr;
        }

        if (!pQueues->Initialize(logicalDevice)) {
            vkDestroyDevice(logicalDevice, nullptr);
            delete pQueues;
            return nullptr;
        }

        auto&& pDevice = new Device(info.pInstance, pQueues, physicalDevice, logicalDevice);

        pDevice->CheckRayTracing(info.rayTracing);

        if (!pDevice->Initialize(info.enableSampleShading, info.multisampling, info.sampleCount)) {
            VK_ERROR("Device::Create() : failed to initialize device!");
            delete pDevice;
            return nullptr;
        }

        VK_LOG("Device::Create() : the device was successfully initialized!");

        return pDevice;
    }

    bool Device::Initialize(bool enableSampleShading, bool multisampling, uint32_t sampleCount) {
        m_enableSampleShading = enableSampleShading;
        m_multisampling = multisampling;

        /// Gather physical device memory properties
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

        VkPhysicalDeviceFeatures2 deviceFeatures = {};
        deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures.pNext = nullptr;

        vkGetPhysicalDeviceFeatures2(m_physicalDevice, &deviceFeatures);
        {
            m_enableSamplerAnisotropy = deviceFeatures.features.samplerAnisotropy;
        }

        m_maxSamplerAnisotropy = Tools::GetMaxSamplerAnisotropy(m_physicalDevice);
        m_deviceName = Tools::GetDeviceName(m_physicalDevice);

        /// device->m_maxCountMSAASamples = calculate...
        if (m_multisampling) {
            VK_LOG("Device::Initialize() : multisampling required...");

            if (sampleCount <= 0) {
                m_maxCountMSAASamples = Tools::GetMaxUsableSampleCount(m_physicalDevice);
            }
            else {
                if (sampleCount == 1) {
                    VK_ERROR("Device::Initialize() : sample count is 1, but multisample disabled!");
                    return false;
                }

                auto&& maxSampleCount = Tools::Convert::SampleCountToInt(Tools::GetMaxUsableSampleCount(m_physicalDevice));
                if (sampleCount > maxSampleCount) {
                    m_maxCountMSAASamples = Tools::Convert::IntToSampleCount(maxSampleCount);
                    VK_LOG("Device::Initialize() : sample count = " + std::to_string(sampleCount) + ", but max sample count " + std::to_string(maxSampleCount));
                    if (m_maxCountMSAASamples == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM) {
                        VK_ERROR("Device::Initialize() : incorrect truncated sample count! Count: " + std::to_string(sampleCount));
                        return false;
                    }
                }
                else {
                    m_maxCountMSAASamples = Tools::Convert::IntToSampleCount(sampleCount);
                    if (m_maxCountMSAASamples == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM) {
                        VK_ERROR("Device::Initialize() : incorrect sample count! Count: " + std::to_string(sampleCount));
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool Device::IsReady() const {
        return m_logicalDevice &&
               m_physicalDevice &&
               m_familyQueues &&
               m_familyQueues->IsReady();
    }

    FamilyQueues* Device::GetQueues() const {
        return m_familyQueues;
    }

    uint32_t Device::GetMemoryType(
            uint32_t typeBits,
            VkMemoryPropertyFlags properties,
            VkBool32 *memTypeFound) const
    {
        for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; ++i) {
            if ((typeBits & 1) == 1) {
                if ((m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    if (memTypeFound) {
                        *memTypeFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }

        if (memTypeFound) {
            *memTypeFound = false;
            return 0;
        }

        VK_ERROR("Device::GetMemoryType() : could not find a matching memory type!");

        return UINT32_MAX;
    }

    bool Device::IsSupportLinearBlitting(const VkFormat& imageFormat) const {
        /// Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);

        return (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    }

    VkCommandPool Device::CreateCommandPool(VkCommandPoolCreateFlags flagBits) const {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {
                VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                nullptr,
                flagBits,
                GetQueues()->GetGraphicsIndex()
            };

        VkCommandPool cmdPool = VK_NULL_HANDLE;
        if (vkCreateCommandPool(*this, &commandPoolCreateInfo, nullptr, &cmdPool) != VK_SUCCESS) {
            VK_ERROR("Device::CreateCommandPool() : failed to create command pool!");
            return VK_NULL_HANDLE;
        }

        return cmdPool;
    }

    uint8_t Device::GetMSAASamplesCount() const {
        return Tools::Convert::SampleCountToInt(m_maxCountMSAASamples);
    }

    bool Device::IsExtensionSupported(const std::string& extension) const {
        return Tools::IsExtensionSupported(m_physicalDevice, extension);
    }

    void Device::CheckRayTracing(bool isRequested) {
        const bool isRayTraceSupport = IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

        if (isRequested) {
            if (isRayTraceSupport) {
                VK_LOG("Device::CheckRayTracing() : ray-tracing was requested and is supported!");

                m_RTProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
                m_RTProps.pNext = nullptr;
                m_RTProps.maxRayRecursionDepth = 0;
                /// m_RTProps.shaderHeaderSize = 0;

                VkPhysicalDeviceProperties2 devProps;
                devProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                devProps.pNext = &m_RTProps;
                devProps.properties = { };

                vkGetPhysicalDeviceProperties2(m_physicalDevice, &devProps);

                VK_LOG(std::string("Device::CheckRayTracing() : VkPhysicalDeviceRayTracingPipelinePropertiesKHR: ")
                   .append("\n\tshaderGroupHandleSize = " + std::to_string(m_RTProps.shaderGroupHandleSize))
                   .append("\n\tmaxRayRecursionDepth = " + std::to_string(m_RTProps.maxRayRecursionDepth))
                   .append("\n\tmaxShaderGroupStride = " + std::to_string(m_RTProps.maxShaderGroupStride))
                   .append("\n\tshaderGroupBaseAlignment = " + std::to_string(m_RTProps.shaderGroupBaseAlignment))
                   .append("\n\tshaderGroupHandleCaptureReplaySize = " + std::to_string(m_RTProps.shaderGroupHandleCaptureReplaySize))
                   .append("\n\tmaxRayDispatchInvocationCount = " + std::to_string(m_RTProps.maxRayDispatchInvocationCount))
                   .append("\n\tshaderGroupHandleAlignment = " + std::to_string(m_RTProps.shaderGroupHandleAlignment))
                   .append("\n\tmaxRayHitAttributeSize = " + std::to_string(m_RTProps.maxRayHitAttributeSize))
                );

                m_rayTracingSupported = true;
            }
            else {
                VK_LOG("Device::CheckRayTracing() : ray-tracing was requested but is not supported!");
            }
        }
        else {
            if (isRayTraceSupport) {
                VK_LOG("Device::CheckRayTracing() : ray-tracing is supported!");
            }
            else {
                VK_LOG("Device::CheckRayTracing() : ray-tracing is not supported!");
            }
        }
    }

    VkFormat Device::GetDepthFormat() const {
        VkFormat depthFormat = VK_FORMAT_UNDEFINED;

        //! Find supported depth format
        //! We prefer 24 bits of depth and 8 bits of stencil, but that may not be supported by all implementations
        //std::vector<VkFormat> depthFormats = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };

        std::vector<VkFormat> depthFormats = {
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
        };

        for (auto& format : depthFormats) {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProps);
            //! Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                depthFormat = format;
                break;
            }
        }

        return depthFormat;
    }

    void Device::WaitQueuesIdle() {
        if (!m_familyQueues) {
            return;
        }

        if (auto&& pQueue = m_familyQueues->GetGraphicsQueue()) {
            vkQueueWaitIdle(pQueue);
        }

        if (auto&& pQueue = m_familyQueues->GetPresentQueue()) {
            vkQueueWaitIdle(pQueue);
        }
    }
}
