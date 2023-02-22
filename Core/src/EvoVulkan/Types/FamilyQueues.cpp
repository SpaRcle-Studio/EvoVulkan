//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/Types/FamilyQueues.h>
#include <EvoVulkan/Types/Surface.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

namespace EvoVulkan::Types {
    FamilyQueues::FamilyQueues(VkPhysicalDevice physicalDevice, const Surface* pSurface)
        : m_physicalDevice(physicalDevice)
        , m_surface(pSurface)
    { }

    FamilyQueues::~FamilyQueues() {
        VK_LOG("FamilyQueues::Destroy() : destroy family queues...");
    }

    bool FamilyQueues::IsComplete() const {
        return
            m_presentQueueFamilyIndex >= 0 &&
            m_graphicsQueueFamilyIndex >= 0 &&
            m_computeQueueFamilyIndex >= 0 &&
            m_transferQueueFamilyIndex >= 0;
    }

    bool FamilyQueues::IsReady() const {
        return
            IsComplete() &&
            m_presentQueue != VK_NULL_HANDLE &&
            m_graphicsQueue != VK_NULL_HANDLE &&
            m_transferQueue != VK_NULL_HANDLE &&
            m_computeQueue  != VK_NULL_HANDLE;
    }

    FamilyQueues* FamilyQueues::Find(VkPhysicalDevice physicalDevice, const Surface* pSurface) {
        /// =================================================[ DEBUG ]==================================================

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
            VK_LOG("FamilyQueues::Find() : found queue family: "
                  "\n\tCount queues: " + std::to_string(queueFamily.queueCount) +
                  "\n\tTimestamp valid bits: " + std::to_string(queueFamily.timestampValidBits) +
                  "\n\tGraphics: " + std::string(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ? "True" : "False") +
                  "\n\tCompute: " + std::string(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT ? "True" : "False") +
                  "\n\tTransfer: " + std::string(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT ? "True" : "False") +
                  "\n\tProtected: " + std::string(queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT ? "True" : "False")
            );
        }

        /// =================================================[ DEBUG ]==================================================

        auto&& pQueues = new FamilyQueues(physicalDevice, pSurface);

        if (!pQueues->FindIndices()) {
            delete pQueues;
            return nullptr;
        }

        return pQueues;
    }

    bool EvoVulkan::Types::FamilyQueues::Initialize(VkDevice logicalDevice) {
        VK_GRAPH("FamilyQueues::Initialize() : initialize family queues queues...");

        m_logicalDevice = logicalDevice;

        vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_logicalDevice, m_computeQueueFamilyIndex, 0, &m_computeQueue);
        vkGetDeviceQueue(m_logicalDevice, m_transferQueueFamilyIndex, 0, &m_transferQueue);
        vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamilyIndex, 0, &m_presentQueue);

        return true;
    }

    bool FamilyQueues::FindIndices() {
        const VkQueueFlagBits askingFlags[3] = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT };
        uint32_t queuesIndices[3] = { ~0u, ~0u, ~0u };

        uint32_t queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        for (size_t i = 0; i < 3; ++i) {
            const VkQueueFlagBits flag = askingFlags[i];
            uint32_t& queueIdx = queuesIndices[i];

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, *m_surface, &presentSupport);

            if (presentSupport) {
                m_presentQueueFamilyIndex = i;
            }

            if (flag == VK_QUEUE_COMPUTE_BIT) {
                for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                    if ((queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                        !(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                        queueIdx = j;
                        break;
                    }
                }
            }
            else if (flag == VK_QUEUE_TRANSFER_BIT) {
                for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                    if ((queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                        !(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                        !(queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                        queueIdx = j;
                        break;
                    }
                }
            }

            if (queueIdx == ~0u) {
                for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                    if (queueFamilyProperties[j].queueFlags & flag) {
                        queueIdx = j;
                        break;
                    }
                }
            }
        }

        m_graphicsQueueFamilyIndex = queuesIndices[0];
        m_computeQueueFamilyIndex = queuesIndices[1];
        m_transferQueueFamilyIndex = queuesIndices[2];

        return true;
    }
}
