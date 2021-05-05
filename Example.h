//
// Created by Nikita on 05.05.2021.
//

#ifndef EVOVULKAN_EXAMPLE_H
#define EVOVULKAN_EXAMPLE_H

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>

#include <EvoVulkan/VulkanKernel.h>

using namespace EvoVulkan;

class VulkanExample : public Core::VulkanKernel {
public:
    void Render() override {
        this->PrepareFrame();

        // Command buffer to be submitted to the queue
        m_submitInfo.commandBufferCount = 1;
        m_submitInfo.pCommandBuffers    = &m_drawCmdBuffs[m_currentBuffer];

        // Submit to queue
        auto result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &m_submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            VK_ERROR("renderFunction() : failed to queue submit!");
            return;
        }

        this->SubmitFrame();
    }

    bool BuildCmdBuffers() override {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.pNext = nullptr;

        // Set clear values for all framebuffer attachments with loadOp set to clear
        // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = m_renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = m_width;
        renderPassBeginInfo.renderArea.extent.height = m_height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = m_clearValues;

        for (int i = 0; i < 3; i++) {
            renderPassBeginInfo.framebuffer = m_frameBuffers[i];

            vkBeginCommandBuffer(m_drawCmdBuffs[i], &cmdBufInfo);
            vkCmdBeginRenderPass(m_drawCmdBuffs[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdEndRenderPass(m_drawCmdBuffs[i]);
            vkEndCommandBuffer(m_drawCmdBuffs[i]);
        }

        return true;
    }
};

#endif //EVOVULKAN_EXAMPLE_H
