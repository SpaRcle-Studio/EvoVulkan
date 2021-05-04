//
// Created by Nikita on 03.05.2021.
//

#ifndef EVOVULKAN_VULKANINITIALIZERS_H
#define EVOVULKAN_VULKANINITIALIZERS_H

#include <vulkan/vulkan.h>

namespace EvoVulkan::Tools::Initializers {
    static VkSubmitInfo SubmitInfo() {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        return submitInfo;
    }

    static VkSemaphoreCreateInfo SemaphoreCreateInfo() {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        return semaphoreCreateInfo;
    }

    static VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0) {
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags             = flags;

        return fenceCreateInfo;
    }

    static VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool& commandPool, const VkCommandBufferLevel& level, uint32_t bufferCount) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool                 = commandPool;
        commandBufferAllocateInfo.level                       = level;
        commandBufferAllocateInfo.commandBufferCount          = bufferCount;

        return commandBufferAllocateInfo;
    }

    static VkImageMemoryBarrier ImageMemoryBarrier() {
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = NULL;

        // Some default values
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        return imageMemoryBarrier;
    }
}

#endif //EVOVULKAN_VULKANINITIALIZERS_H
