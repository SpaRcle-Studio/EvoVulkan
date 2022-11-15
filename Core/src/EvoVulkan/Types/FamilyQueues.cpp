//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/Types/FamilyQueues.h>
#include <EvoVulkan/Types/Surface.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

[[nodiscard]] bool EvoVulkan::Types::FamilyQueues::IsComplete() const {
    return m_iGraphics >= 0 && m_iPresent >= 0;
}

bool EvoVulkan::Types::FamilyQueues::IsReady() const {
    //return IsComplete() && (m_presentQueue != VK_NULL_HANDLE && m_graphicsQueue != VK_NULL_HANDLE);
    return IsComplete() && (m_graphicsQueue != VK_NULL_HANDLE);
}

EvoVulkan::Types::FamilyQueues* EvoVulkan::Types::FamilyQueues::Find(
        VkPhysicalDevice const &device,
        const EvoVulkan::Types::Surface *surface)
{
    auto* queues = new FamilyQueues();

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
        VK_LOG("FamilyQueues::Find() : Found queue family: "
                          "\n\tCount queues: " + std::to_string(queueFamily.queueCount) +
                          "\n\tTimestamp valid bits: " + std::to_string(queueFamily.timestampValidBits) +
                          "\n\tGraphics: " + std::string(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ? "True" : "False") +
                          "\n\tCompute: " + std::string(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT ? "True" : "False") +
                          "\n\tTransfer: " + std::string(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT ? "True" : "False") +
                          "\n\tProtected: " + std::string(queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT ? "True" : "False"));
    }

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queues->m_iGraphics = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *surface, &presentSupport);

        if (presentSupport)
            queues->m_iPresent = i;

        if (queues->IsComplete())
            break;

        i++;
    }

    return queues;
}

void EvoVulkan::Types::FamilyQueues::Destroy() {
    VK_LOG("FamilyQueues::Destroy() : destroy family queues...");

    if (!IsReady())
        return;

    m_graphicsQueue = VK_NULL_HANDLE;
    m_presentQueue  = VK_NULL_HANDLE;

    m_iPresent  = -2;
    m_iGraphics = -2;
}

void EvoVulkan::Types::FamilyQueues::Free() {
    VK_LOG("FamilyQueues::Free() : free family queues pointer...");

    delete this;
}
