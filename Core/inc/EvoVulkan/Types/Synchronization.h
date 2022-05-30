//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SYNCHRONIZATION_H
#define EVOVULKAN_SYNCHRONIZATION_H

#include <EvoVulkan/Tools/NonCopyable.h>

#ifdef EVK_MINGW
    #pragma GCC diagnostic ignored "-Wattributes"
#endif

namespace EvoVulkan::Types {
    struct DLL_EVK_EXPORT Synchronization {
        // Swap chain image presentation
        VkSemaphore  m_presentComplete;
        // Command buffer submission and execution
        VkSemaphore  m_renderComplete;

        [[nodiscard]] inline bool IsReady() const {
            return m_presentComplete != VK_NULL_HANDLE && m_renderComplete != VK_NULL_HANDLE;
        }
    };
}

#endif //EVOVULKAN_SYNCHRONIZATION_H
