//
// Created by Nikita on 13.05.2021.
//

#ifndef EVOVULKAN_VULKANINSERT_H
#define EVOVULKAN_VULKANINSERT_H

#include <vulkan/vulkan.h>

namespace EvoVulkan::Tools::Insert {
    EVK_MAYBE_UNUSED static VkRenderPassBeginInfo RenderPassBeginInfo(
            const uint32_t width,
            const uint32_t height,
            const VkRenderPass& renderPass,
            const VkFramebuffer& framebuffer,
            VkClearValue* clearValues,
            const uint32_t countValues)
    {
        return VkRenderPassBeginInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext           = nullptr,
            .renderPass      = renderPass,
            .framebuffer     = framebuffer,
            .renderArea      = { VkOffset2D(), { width, height } },
            .clearValueCount = countValues,
            .pClearValues    = clearValues
        };
    }

    EVK_MAYBE_UNUSED static void ImageMemoryBarrier(
            VkCommandBuffer command_buffer,
            VkImage image,
            VkAccessFlags src_access_mask,
            VkAccessFlags dst_access_mask,
            VkImageLayout old_layout,
            VkImageLayout new_layout,
            VkPipelineStageFlags src_stage_mask,
            VkPipelineStageFlags dst_stage_mask,
            VkImageSubresourceRange subresource_range) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = src_access_mask;
        barrier.dstAccessMask = dst_access_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = image;
        barrier.subresourceRange = subresource_range;

        vkCmdPipelineBarrier(
                command_buffer,
                src_stage_mask,
                dst_stage_mask,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
    }
}

#endif //EVOVULKAN_VULKANINSERT_H
