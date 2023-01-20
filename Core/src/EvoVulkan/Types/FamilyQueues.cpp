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

        m_graphicsQueue = VK_NULL_HANDLE;
        m_presentQueue  = VK_NULL_HANDLE;

        m_iPresent  = -2;
        m_iGraphics = -2;
    }

    bool FamilyQueues::IsComplete() const {
        return m_iGraphics >= 0 && m_iPresent >= 0;
    }

    bool FamilyQueues::IsReady() const {
        return IsComplete() && (m_graphicsQueue != VK_NULL_HANDLE);
    }

    FamilyQueues* FamilyQueues::Find(VkPhysicalDevice physicalDevice, const Surface* pSurface) {
        auto&& pQueues = new FamilyQueues(physicalDevice, pSurface);

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

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                pQueues->m_iGraphics = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, *pSurface, &presentSupport);

            if (presentSupport) {
                pQueues->m_iPresent = i;
            }

            if (pQueues->IsComplete()) {
                break;
            }

            ++i;
        }

        return pQueues;
    }

    bool EvoVulkan::Types::FamilyQueues::Initialize(VkDevice logicalDevice) {
        m_logicalDevice = logicalDevice;

        VK_GRAPH("FamilyQueues::Initialize() : initialize family queues queues...");

        VkQueue graphics = VK_NULL_HANDLE;
        /// VkQueue present = VK_NULL_HANDLE;

        vkGetDeviceQueue(logicalDevice, GetGraphicsIndex(), 0, &graphics);
        /// vkGetDeviceQueue(logicalDevice, queues->GetPresentIndex(), 1, &present);

        m_graphicsQueue = graphics;
        /// queues->SetPresentQueue(present);

        return true;
    }
}
