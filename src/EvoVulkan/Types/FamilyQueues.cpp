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
            (!m_surface || m_presentQueueFamilyIndex >= 0) &&
            m_graphicsQueueFamilyIndex >= 0 &&
            m_computeQueueFamilyIndex >= 0;

        /// m_transferQueueFamilyIndex есть далеко не на всех девайсах, не учитываем ее, опциональное.
    }

    bool FamilyQueues::IsReady() const {
        return
            IsComplete() &&
            (!m_surface ||m_presentQueue != VK_NULL_HANDLE) &&
            m_graphicsQueue != VK_NULL_HANDLE &&
            m_computeQueue  != VK_NULL_HANDLE;

        /// m_transferQueue есть далеко не на всех девайсах, не учитываем ее, опциональное.
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
        VK_GRAPH("FamilyQueues::Initialize() : initializing family queues queues...");

        m_logicalDevice = logicalDevice;

        VK_LOG("FamilyQueues::Initialize() : "
           "\n\tGraphics queue index: " + std::to_string(m_graphicsQueueFamilyIndex) +
           "\n\tCompute queue index: " + std::to_string(m_computeQueueFamilyIndex) +
           "\n\tTransfer queue index: " + std::to_string(m_transferQueueFamilyIndex) +
           "\n\tPresent queue index: " + std::to_string(m_presentQueueFamilyIndex)
        );

        if (m_graphicsQueueFamilyIndex >= 0) {
            vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
        }

        if (m_computeQueueFamilyIndex >= 0) {
            vkGetDeviceQueue(m_logicalDevice, m_computeQueueFamilyIndex, 0, &m_computeQueue);
        }

        if (m_transferQueueFamilyIndex >= 0) {
            vkGetDeviceQueue(m_logicalDevice, m_transferQueueFamilyIndex, 0, &m_transferQueue);
        }

        if (m_surface && m_presentQueueFamilyIndex >= 0) {
            vkGetDeviceQueue(m_logicalDevice, m_presentQueueFamilyIndex, 0, &m_presentQueue);
        }

        if (!m_graphicsQueue || m_graphicsQueueFamilyIndex < 0) {
            VK_ERROR("FamilyQueues::Initialize() : graphics queue is not supported!");
            return false;
        }

        if (!m_computeQueue || m_computeQueueFamilyIndex < 0) {
            VK_ERROR("FamilyQueues::Initialize() : compute queue is not supported!");
            return false;
        }

        if (m_surface && (!m_presentQueue || m_presentQueueFamilyIndex < 0)) {
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

        for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
            const VkQueueFamilyProperties& qf = m_queueFamilyProperties[j];
            VK_LOG(EVK_FORMAT("FamilyQueues::FindIndices() : queue family %u:"
                   "\n\tCount: %u"
                   "\n\tGraphics: %s"
                   "\n\tCompute: %s"
                   "\n\tTransfer: %s"
                   "\n\tProtected: %s"
                   "\n\tGranularity: %ux%ux%u",
                   j,
                   qf.queueCount,
                   (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "True" : "False",
                   (qf.queueFlags & VK_QUEUE_COMPUTE_BIT)  ? "True" : "False",
                   (qf.queueFlags & VK_QUEUE_TRANSFER_BIT) ? "True" : "False",
                   (qf.queueFlags & VK_QUEUE_PROTECTED_BIT)? "True" : "False",
                   qf.minImageTransferGranularity.width,
                   qf.minImageTransferGranularity.height,
                   qf.minImageTransferGranularity.depth));
        }

        // Graphics: просто берём первую с графикой
        for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
            if (m_queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queuesIndices[0] = j;
                break;
            }
        }

        // Compute: ищем compute-only, иначе берём любую с compute
        for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
            if ((m_queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(m_queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                queuesIndices[1] = j;
                break;
            }
        }
        if (queuesIndices[1] == ~0u) {
            for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
                if (m_queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    queuesIndices[1] = j;
                    break;
                }
            }
        }

        // Transfer: ищем transfer-only, иначе любую с transfer
        for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
            if ((m_queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(m_queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(m_queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                queuesIndices[2] = j;
                break;
            }
        }
        if (queuesIndices[2] == ~0u) {
            for (uint32_t j = 0; j < m_queueFamilyPropertyCount; ++j) {
                if (m_queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    queuesIndices[2] = j;
                    break;
                }
            }
        }

        // Present support
        if (m_surface) {
            for (uint32_t i = 0; i < m_queueFamilyProperties.size(); ++i) {
                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, *m_surface, &presentSupport);
                if (presentSupport) {
                    m_presentQueueFamilyIndex = static_cast<int32_t>(i);
                    break;
                }
            }
        }

        m_graphicsQueueFamilyIndex = static_cast<int32_t>(queuesIndices[0]);
        m_computeQueueFamilyIndex  = static_cast<int32_t>(queuesIndices[1]);
        m_transferQueueFamilyIndex = static_cast<int32_t>(queuesIndices[2]);

        VK_LOG(EVK_FORMAT("FamilyQueues::FindIndices() : selected queue families: G=%i, C=%i, T=%i, Present=%i",
               m_graphicsQueueFamilyIndex,
               m_computeQueueFamilyIndex,
               m_transferQueueFamilyIndex,
               m_presentQueueFamilyIndex));

        return (m_graphicsQueueFamilyIndex != -1);
    }
}
