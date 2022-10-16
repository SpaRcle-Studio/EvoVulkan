//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/Types/Device.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/DeviceTools.h>

EvoVulkan::Types::Device *EvoVulkan::Types::Device::Create(const EvoDeviceCreateInfo& info) {
    if (info.physicalDevice == VK_NULL_HANDLE) {
        VK_ERROR("Device::Create() : physical device is nullptr!");
        return nullptr;
    }

    if (info.logicalDevice == VK_NULL_HANDLE) {
        VK_ERROR("Device::Create() : logical device is nullptr!");
        return nullptr;
    }

    if (info.familyQueues == nullptr) {
        VK_ERROR("Device::Create() : family queues is nullptr!");
        return nullptr;
    }

    auto* device = new Device();

    device->m_instance            = info.instance;
    device->m_physicalDevice      = info.physicalDevice;
    device->m_logicalDevice       = info.logicalDevice;
    device->m_familyQueues        = info.familyQueues;
    device->m_enableSampleShading = info.enableSampleShading;

    /// Gather physical device memory properties
    vkGetPhysicalDeviceMemoryProperties(info.physicalDevice, &device->m_memoryProperties);

    VkPhysicalDeviceFeatures2 deviceFeatures = {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = nullptr;

    vkGetPhysicalDeviceFeatures2(info.physicalDevice, &deviceFeatures);
    {
        device->m_enableSamplerAnisotropy = deviceFeatures.features.samplerAnisotropy;
    }

    device->m_maxSamplerAnisotropy = Tools::GetMaxSamplerAnisotropy(*device);
    device->m_deviceName = Tools::GetDeviceName(info.physicalDevice);

    /// device->m_maxCountMSAASamples = calculate...
    if (info.multisampling) {
        if (info.sampleCount <= 0) {
            device->m_maxCountMSAASamples = Tools::GetMaxUsableSampleCount(device->m_physicalDevice);
        }
        else {
            if (info.sampleCount == 1) {
                VK_ERROR("Device::Create() : sample count is 1, but multisample disabled!");
                return nullptr;
            }

            auto&& maxSampleCount = Tools::Convert::SampleCountToInt(Tools::GetMaxUsableSampleCount(device->m_physicalDevice));
            if (info.sampleCount > maxSampleCount) {
                device->m_maxCountMSAASamples = Tools::Convert::IntToSampleCount(maxSampleCount);
                VK_LOG("Device::Create() : sample count = " + std::to_string(info.sampleCount) + ", but max sample count " + std::to_string(maxSampleCount));
                if (device->m_maxCountMSAASamples == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM) {
                    VK_ERROR("Device::Create() : incorrect truncated sample count! Count: " + std::to_string(info.sampleCount));
                    return nullptr;
                }
            }
            else {
                device->m_maxCountMSAASamples = Tools::Convert::IntToSampleCount(info.sampleCount);
                if (device->m_maxCountMSAASamples == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM) {
                    VK_ERROR("Device::Create() : incorrect sample count! Count: " + std::to_string(info.sampleCount));
                    return nullptr;
                }
            }
        }
    }

    return device;
}

void EvoVulkan::Types::Device::Free() {
    VK_LOG("Device::Free() : free device pointer...");

    delete this;
}

bool EvoVulkan::Types::Device::IsReady() const {
    return  m_logicalDevice &&
            m_physicalDevice &&
            m_familyQueues &&
            m_familyQueues->IsReady();
}

bool EvoVulkan::Types::Device::Destroy() {
    VK_LOG("Device::Destroy() : destroying vulkan device...");

    if (!IsReady()) {
        VK_ERROR("Device::Destroy() : device isn't ready!");
        return false;
    }

    this->m_familyQueues->Destroy();
    this->m_familyQueues->Free();
    this->m_familyQueues = nullptr;

    vkDestroyDevice(m_logicalDevice, nullptr);
    this->m_logicalDevice = VK_NULL_HANDLE;

    return true;
}

EvoVulkan::Types::FamilyQueues *EvoVulkan::Types::Device::GetQueues() const {
    return m_familyQueues;
}

uint32_t EvoVulkan::Types::Device::GetMemoryType(
        uint32_t typeBits,
        VkMemoryPropertyFlags properties,
        VkBool32 *memTypeFound) const
{
    for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++) {
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
    } else {
        VK_ERROR("Device::GetMemoryType() : Could not find a matching memory type");
        return UINT32_MAX;
    }
}

bool EvoVulkan::Types::Device::IsSupportLinearBlitting(const VkFormat& imageFormat) const {
    /// Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);

    return (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
}

VkCommandPool EvoVulkan::Types::Device::CreateCommandPool(VkCommandPoolCreateFlags flagBits) const {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            nullptr,
            flagBits,
            this->GetQueues()->GetGraphicsIndex()
        };

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(*this, &commandPoolCreateInfo, nullptr, &cmdPool) != VK_SUCCESS) {
        VK_ERROR("Device::CreateCommandPool() : failed to create command pool!");
        return VK_NULL_HANDLE;
    }

    return cmdPool;
}

uint8_t EvoVulkan::Types::Device::GetMSAASamplesCount() const {
    return Tools::Convert::SampleCountToInt(m_maxCountMSAASamples);
}

