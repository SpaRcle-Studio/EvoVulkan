//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SYNCHRONIZATION_H
#define EVOVULKAN_SYNCHRONIZATION_H

#include <vulkan/vulkan.h>

namespace EvoVulkan::Types {
    struct Synchronization {
        // Swap chain image presentation
        VkSemaphore  m_presentComplete;
        // Command buffer submission and execution
        VkSemaphore  m_renderComplete;

        VkSubmitInfo m_submitInfo;

        [[nodiscard]] inline bool IsReady() const {
            return m_presentComplete != VK_NULL_HANDLE && m_renderComplete != VK_NULL_HANDLE;
        }
    };
}

#endif //EVOVULKAN_SYNCHRONIZATION_H
