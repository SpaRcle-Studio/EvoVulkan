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
#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Types {
    struct RenderPass {
        VkRenderPass m_self;
        uint32_t m_countAttachments;
        uint32_t m_countColorAttach;

        [[nodiscard]] bool Ready() const noexcept { return m_countAttachments > 0 && m_self != VK_NULL_HANDLE; }
    };

    static void DestroyRenderPass(const Types::Device *device, RenderPass *renderPass) {
        VK_LOG("Tools::DestroyRenderPass() : destroy vulkan render pass...");

        if (renderPass->Ready()) {
            vkDestroyRenderPass(*device, renderPass->m_self, nullptr);
            renderPass->m_self = VK_NULL_HANDLE;
            renderPass->m_countAttachments = 0;
            renderPass->m_countColorAttach = 0;
        } else
            VK_ERROR("Tools::DestroyRenderPass() : render pass is nullptr!");
    }

    static RenderPass CreateRenderPass(const Types::Device *device, const Types::Swapchain *swapchain,
                                       std::vector<VkAttachmentDescription> attachments = {}, bool multisampling = false) {
        VK_GRAPH("Types::CreateRenderPass() : create vulkan render pass...");

        VkSubpassDescription subpassDescription = {};
        std::vector<VkAttachmentReference> colorReferences = {};
        std::vector<VkAttachmentReference> resolveReferences = {};
        VkAttachmentReference depthReference = {};
        // Resolve attachment reference for the color attachment

        if (attachments.empty()) {
            attachments.resize(multisampling ? 3 : 2);

            // Color attachment
            attachments[0].format = swapchain->GetColorFormat();
            attachments[0].samples = device->GetMSAASamples();
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            if (multisampling) {
                // This is the frame buffer attachment to where the multisampled image
                // will be resolved to and which will be presented to the swapchain
                attachments[1].format = swapchain->GetColorFormat();
                attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                // Multisampled depth attachment we render to
                attachments[2].format = swapchain->GetDepthFormat();
                attachments[2].samples = device->GetMSAASamples();
                attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else {
                // Depth attachment
                attachments[1].format = swapchain->GetDepthFormat();
                attachments[1].samples = device->GetMSAASamples();
                attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            depthReference = { multisampling ? 2u : 1u, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            resolveReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        } else {
            //uint32_t bind = 0;
            for (uint32_t i = 0; i < attachments.size() - 1; i++) {
                colorReferences.push_back({i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                if (multisampling) {
                    VkAttachmentDescription attachmentDescription = {
                            .format = attachments[i].format,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    };

                    attachments.insert(attachments.begin() + i + 1, attachmentDescription);
                    i++;
                    resolveReferences.push_back({ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                }
            }

            //resolveReference = { static_cast<uint32_t>(attachments.size() - 2), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            depthReference = { static_cast<uint32_t>(colorReferences.size() + resolveReferences.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        }

        /*
         * [Error] Validation Error: [ VUID-vkCmdBeginRenderPass-initialLayout-00897 ] Object 0: handle = 0x5261630000000026, type
            = VK_OBJECT_TYPE_IMAGE; Object 1: handle = 0x8483000000000025, type = VK_OBJECT_TYPE_RENDER_PASS; Object 2: handle = 0x7
            925100000000035, type = VK_OBJECT_TYPE_FRAMEBUFFER; Object 3: handle = 0xab46ad0000000028, type = VK_OBJECT_TYPE_IMAGE_V
            IEW; | MessageID = 0x961074c1 | vkCmdBeginRenderPass(): Layout/usage mismatch for attachment 0 in VkRenderPass 0x8483000
            000000025[] - the final layout is VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL but the image attached to VkFramebuffer 0x792
            5100000000035[] via VkImageView 0xab46ad0000000028[] was not created with VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT or VK_IMAG
            E_USAGE_SAMPLED_BIT The Vulkan spec states: If any of the initialLayout or finalLayout member of the VkAttachmentDescrip
            tion structures or the layout member of the VkAttachmentReference structures specified when creating the render pass spe
            cified in the renderPass member of pRenderPassBegin is VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL then the corresponding a
            ttachment image view of the framebuffer specified in the framebuffer member of pRenderPassBegin must have been created w
            ith a usage value including VK_IMAGE_USAGE_SAMPLED_BIT or VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
         */

        {
            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
            subpassDescription.pColorAttachments = colorReferences.data();
            subpassDescription.pDepthStencilAttachment = &depthReference;
            subpassDescription.inputAttachmentCount = 0;
            subpassDescription.pInputAttachments = nullptr;
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments = nullptr;
            subpassDescription.pResolveAttachments = multisampling ? resolveReferences.data() : nullptr;
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

        return { renderPass, (uint32_t)attachments.size(), (uint32_t)colorReferences.size() };
    }
}

#endif //EVOVULKAN_RENDERPASS_H
