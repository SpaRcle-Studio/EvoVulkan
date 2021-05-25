//
// Created by Nikita on 24.05.2021.
//

#ifndef EVOVULKAN_RENDERPASS_H
#define EVOVULKAN_RENDERPASS_H

#include <EvoVulkan/Tools/VulkanHelper.h>
#include <array>
#include <vector>

#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Types {
    struct RenderPass {
        VkRenderPass m_self;
        uint32_t m_countColorAttach;

        [[nodiscard]] bool Ready() const noexcept { return m_countColorAttach > 0 && m_self != VK_NULL_HANDLE; }
    };

    static void DestroyRenderPass(const Types::Device *device, RenderPass *renderPass) {
        VK_LOG("Tools::DestroyRenderPass() : destroy vulkan render pass...");

        if (renderPass->Ready()) {
            vkDestroyRenderPass(*device, renderPass->m_self, nullptr);
            renderPass->m_self = VK_NULL_HANDLE;
            renderPass->m_countColorAttach = 0;
        } else
            VK_ERROR("Tools::DestroyRenderPass() : render pass is nullptr!");
    }

    static RenderPass CreateRenderPass(const Types::Device *device, const Types::Swapchain *swapchain,
                                       std::vector<VkAttachmentDescription> attachments = {}) {
        VK_GRAPH("Types::CreateRenderPass() : create vulkan render pass...");

        VkSubpassDescription subpassDescription = {};
        std::vector<VkAttachmentReference> colorReferences = {};
        VkAttachmentReference depthReference = {};

        if (attachments.empty()) {
            attachments.resize(2);

            // Color attachment
            attachments[0].format = swapchain->GetColorFormat();
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT; // TODO: 1 sample!
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            // Depth attachment
            attachments[1].format = swapchain->GetDepthFormat();
            attachments[1].samples = VK_SAMPLE_COUNT_1_BIT; // TODO: 1 sample!
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            depthReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount = 1;
            subpassDescription.pColorAttachments = colorReferences.data();
            subpassDescription.pDepthStencilAttachment = &depthReference;
            subpassDescription.inputAttachmentCount = 0;
            subpassDescription.pInputAttachments = nullptr;
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments = nullptr;
            subpassDescription.pResolveAttachments = nullptr;
        } else {
            for (uint32_t i = 0; i < attachments.size() - 1; i++)
                colorReferences.push_back({i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

            depthReference = {static_cast<uint32_t>(attachments.size() - 1),
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
            subpassDescription.pColorAttachments = colorReferences.data();
            subpassDescription.pDepthStencilAttachment = &depthReference;
            subpassDescription.inputAttachmentCount = 0;
            subpassDescription.pInputAttachments = nullptr;
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments = nullptr;
            subpassDescription.pResolveAttachments = nullptr;
        }

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies = {};

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // TODO: remove read?
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // TODO: remove read?
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VkRenderPass renderPass = VK_NULL_HANDLE;
        auto result = vkCreateRenderPass(*device, &renderPassInfo, nullptr, &renderPass);
        if (result != VK_SUCCESS) {
            VK_ERROR("Types::CreateRenderPass() : failed to create vulkan render pass! Reason: " +
                     Tools::Convert::result_to_description(result));
            return {};
        }

        return {renderPass, (uint32_t) colorReferences.size()};
    }
}

#endif //EVOVULKAN_RENDERPASS_H
