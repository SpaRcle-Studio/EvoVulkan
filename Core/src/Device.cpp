//
// Created by Nikita on 12.04.2021.
//

#include "EvoVulkan/Types/Device.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

EvoVulkan::Types::Device *EvoVulkan::Types::Device::Create(
        const VkPhysicalDevice& physicalDevice,
        const VkDevice& logicalDevice,
        FamilyQueues* familyQueues,
        const bool& enableSampleShading)
{
    if (physicalDevice == VK_NULL_HANDLE) {
        Tools::VkDebug::Error("Device::Create() : physical device is nullptr!");
        return nullptr;
    }

    if (logicalDevice == VK_NULL_HANDLE) {
        Tools::VkDebug::Error("Device::Create() : logical device is nullptr!");
        return nullptr;
    }

    if (familyQueues == nullptr) {
        Tools::VkDebug::Error("Device::Create() : family queues is nullptr!");
        return nullptr;
    }

    auto* device = new Device();

    device->m_physicalDevice      = physicalDevice;
    device->m_logicalDevice       = logicalDevice;
    device->m_familyQueues        = familyQueues;
    device->m_enableSampleShading = enableSampleShading;

    // Gather physical device memory properties
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &device->m_memoryProperties);

    //device->m_maxCountMSAASamples = calculate...

    return device;
}

void EvoVulkan::Types::Device::Free() {
    Tools::VkDebug::Log("Device::Free() : free device pointer...");

    delete this;
}

bool EvoVulkan::Types::Device::IsReady() const {
    return  m_logicalDevice &&
            m_physicalDevice &&
            m_familyQueues &&
            m_familyQueues->IsReady();
}

bool EvoVulkan::Types::Device::Destroy() {
    Tools::VkDebug::Log("Device::Destroy() : destroying vulkan device...");

    if (!this->IsReady()) {
        Tools::VkDebug::Error("Device::Destroy() : device isn't ready!");
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
