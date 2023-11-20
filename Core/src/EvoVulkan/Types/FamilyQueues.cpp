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

        VK_LOG("FamilyQueues::Initialize() : "
           "\n\tGraphics queue index: " + std::to_string(m_graphicsQueueFamilyIndex) +
           "\n\tCompute queue index: " + std::to_string(m_computeQueueFamilyIndex) +
           "\n\tTransfer queue index: " + std::to_string(m_transferQueueFamilyIndex) +
           "\n\tPresent queue index: " + std::to_string(m_presentQueueFamilyIndex)
       );

        vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_logicalDevice, m_computeQueueFamilyIndex, 0, &m_computeQueue);
        vkGetDeviceQueue(m_logicalDevice, m_transferQueueFamilyIndex, 0, &m_transferQueue);
        vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamilyIndex, 0, &m_presentQueue);

        if (!m_graphicsQueue || m_graphicsQueueFamilyIndex < 0) {
            VK_ERROR("FamilyQueues::Initialize() : graphics queue is not supported!");
            return false;
        }

        if (!m_computeQueue || m_computeQueueFamilyIndex < 0) {
            VK_ERROR("FamilyQueues::Initialize() : compute queue is not supported!");
            return false;
        }

        if (!m_transferQueue || m_transferQueueFamilyIndex < 0) {
            VK_ERROR("FamilyQueues::Initialize() : transfer queue is not supported!");
            return false;
        }

        if (!m_presentQueue || m_presentQueueFamilyIndex < 0) {
            VK_ERROR("FamilyQueues::Initialize() : present queue is not supported!");
            return false;
        }

        return true;
    }

    bool FamilyQueues::FindIndices() {
        const VkQueueFlagBits askingFlags[3] = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT };
        uint32_t queuesIndices[3] = { ~0u, ~0u, ~0u };

        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &m_queueFamilyPropertyCount, nullptr);
        m_queueFamilyProperties.resize(m_queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &m_queueFamilyPropertyCount, m_queueFamilyProperties.data());

        for (const VkQueueFamilyProperties& queueFamily : m_queueFamilyProperties) {
            VK_LOG("FamilyQueues::FindIndices() : found queue family: "
                   "\n\tCount queues: " + std::to_string(queueFamily.queueCount) +
                   "\n\tTimestamp valid bits: " + std::to_string(queueFamily.timestampValidBits) +
                   "\n\tGraphics: " + std::string(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ? "True" : "False") +
                   "\n\tCompute: " + std::string(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT ? "True" : "False") +
                   "\n\tTransfer: " + std::string(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT ? "True" : "False") +
                   "\n\tProtected: " + std::string(queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT ? "True" : "False") +
                   "\n\tMin image transfer granularity: "
                        "\n\t\tWidth: " + std::to_string(queueFamily.minImageTransferGranularity.width) +
                        "\n\t\tHeight: " + std::to_string(queueFamily.minImageTransferGranularity.height) +
                        "\n\t\tDepth: " + std::to_string(queueFamily.minImageTransferGranularity.depth)
            );
        }

        for (size_t i = 0; i < 3; ++i) {
            const VkQueueFlagBits flag = askingFlags[i];
            uint32_t& queueIdx = queuesIndices[i];

            if (flag == VK_QUEUE_COMPUTE_BIT) {
                for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
                    if ((m_queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                        !(m_queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                        queueIdx = j;
                        break;
                    }
                }
            }
            else if (flag == VK_QUEUE_TRANSFER_BIT) {
                for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
                    if ((m_queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                        !(m_queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                        !(m_queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                        queueIdx = j;
                        break;
                    }
                }
            }

            if (queueIdx == ~0u) {
                for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
                    if (m_queueFamilyProperties[j].queueFlags & flag) {
                        queueIdx = j;
                        break;
                    }
                }
            }
        }

        for (uint32_t i = 0; i < m_queueFamilyProperties.size(); ++i) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, *m_surface, &presentSupport);

            if (presentSupport) {
                m_presentQueueFamilyIndex = static_cast<int32_t>(i);
                break;
            }
        }

        m_graphicsQueueFamilyIndex = static_cast<int32_t>(queuesIndices[0]);
        m_computeQueueFamilyIndex = static_cast<int32_t>(queuesIndices[1]);
        m_transferQueueFamilyIndex = static_cast<int32_t>(queuesIndices[2]);

        return true;
    }
}
