//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_PIPELINE_H
#define EVOVULKAN_PIPELINE_H

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Types {
    struct DLL_EVK_EXPORT Pipeline {
        VkPipeline            m_pipeline            = VK_NULL_HANDLE;
        VkPipelineLayout      m_layout              = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    };
}

#endif //EVOVULKAN_PIPELINE_H
