//
// Created by Nikita on 05.05.2021.
//

#include <EvoVulkan/Tools/VulkanTools.h>

namespace EvoVulkan::Tools {
    void DestroyPipelineCache(const VkDevice& device, VkPipelineCache* cache) {
        if (!cache || *cache == VK_NULL_HANDLE) {
            VK_ERROR("Tools::DestroyPipelineCache() : cache is nullptr!");
            return;
        }

        vkDestroyPipelineCache(device, *cache, nullptr);
        *cache = VK_NULL_HANDLE;
    }

    VkPipelineCache CreatePipelineCache(const VkDevice& device) {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        auto result = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);

        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreatePipelineCache() : failed to create pipeline cache! Reason:" +
                     Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return pipelineCache;
    }

    void DestroySynchronization(const VkDevice& device, Types::Synchronization* sync) {
        VK_LOG("Tools::DestroySynchronization() : destroy vulkan synchronizations...");

        if (!sync->IsReady()) {
            VK_ERROR("Tools::DestroySynchronization() : synchronizations isn't ready!");
            return;
        }

        vkDestroySemaphore(device, sync->m_presentComplete, nullptr);
        vkDestroySemaphore(device, sync->m_renderComplete, nullptr);

        sync->m_presentComplete = VK_NULL_HANDLE;
        sync->m_renderComplete  = VK_NULL_HANDLE;
    }

    Types::Synchronization CreateSynchronization(const VkDevice& device) {
        VK_GRAPH("Tools::CreateSynchronization() : create vulkan synchronizations...");

        Types::Synchronization sync = {};

        // Create synchronization objects
        VkSemaphoreCreateInfo semaphoreCreateInfo = Initializers::SemaphoreCreateInfo();
        // Create a semaphore used to synchronize image presentation
        // Ensures that the image is displayed before we start submitting new commands to the queue
        auto result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sync.m_presentComplete);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateSynchronization() : failed to create present semaphore!");
            return {};
        }
        // Create a semaphore used to synchronize command submission
        // Ensures that the image is not presented until all commands have been submitted and executed
        result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sync.m_renderComplete);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateSynchronization() : failed to create render semaphore!");
            return {};
        }

        return sync;
    }
}