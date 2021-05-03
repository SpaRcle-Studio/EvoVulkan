//
// Created by Nikita on 03.05.2021.
//

#ifndef EVOVULKAN_VULKANINITIALIZERS_H
#define EVOVULKAN_VULKANINITIALIZERS_H

#include <vulkan/vulkan.h>

namespace EvoVulkan::Tools::Initializers {
    static VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool& commandPool, const VkCommandBufferLevel& level, uint32_t bufferCount) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool                 = commandPool;
        commandBufferAllocateInfo.level                       = level;
        commandBufferAllocateInfo.commandBufferCount          = bufferCount;

        return commandBufferAllocateInfo;
    }

}

#endif //EVOVULKAN_VULKANINITIALIZERS_H
