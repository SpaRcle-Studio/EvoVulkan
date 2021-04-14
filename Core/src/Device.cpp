//
// Created by Nikita on 12.04.2021.
//

#include "EvoVulkan/Types/Device.h"

EvoVulkan::Types::Device *EvoVulkan::Types::Device::Create(VkPhysicalDevice physicalDevice) {
    auto* device = new Device();

    device->m_physicalDevice = physicalDevice;

    return device;
}

bool EvoVulkan::Types::Device::Free() {
    return false;
}
