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
private:
    //VkPipeline            m_pipeline      = VK_NULL_HANDLE;

    VkPipelineLayout      m_pipelineLayout      = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet       m_descriptorSet       = VK_NULL_HANDLE;

    struct {
        VkPipelineVertexInputStateCreateInfo inputState;
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    } m_vertices;
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

    bool SetupVertices() {
        //m_vertices.bindingDescriptions.resize(1);
        //m_vertices.bindingDescriptions[0] =
        //        Tools::Initializers::VertexInputBindingDescription(
        //                0, //VERTEX_BUFFER_BIND_ID
        //                sizeof(Vertex),
        //                VK_VERTEX_INPUT_RATE_VERTEX);
    }

    bool SetupShader() {
        this->m_descriptorSetLayout = Tools::CreateDescriptorLayout(*m_device, {
               Tools::Initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        });
        if (this->m_descriptorSetLayout == VK_NULL_HANDLE) {
            VK_ERROR("VulkanExample::SetupShader() : failed to create descriptor layout!");
            return false;
        }

        this->m_pipelineLayout = Tools::CreatePipelineLayout(*m_device, m_descriptorSetLayout);
        if (this->m_pipelineLayout == VK_NULL_HANDLE) {
            VK_ERROR("VulkanExample::SetupShader() : failed to create pipeline layout!");
            return false;
        }

        this->m_descriptorSet = this->m_descriptorManager->AllocateDescriptor(m_descriptorSetLayout);
        this->m_descriptorSet = this->m_descriptorManager->AllocateDescriptor(m_descriptorSetLayout);
        this->m_descriptorSet = this->m_descriptorManager->AllocateDescriptor(m_descriptorSetLayout);

        return true;
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

    bool Destroy() override {
        if (m_pipelineLayout)
            vkDestroyPipelineLayout(*m_device, m_pipelineLayout, nullptr);

        if (m_descriptorSetLayout)
            vkDestroyDescriptorSetLayout(*m_device, m_descriptorSetLayout, nullptr);

        return VulkanKernel::Destroy();
    }
};

#endif //EVOVULKAN_EXAMPLE_H
