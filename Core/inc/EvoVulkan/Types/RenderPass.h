//
// Created by Nikita on 24.05.2021.
//

#ifndef EVOVULKAN_RENDERPASS_H
#define EVOVULKAN_RENDERPASS_H

#include <EvoVulkan/Tools/VulkanHelper.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Types {
    struct DLL_EVK_EXPORT RenderPass {
        VkRenderPass m_self;
        uint32_t m_countAttachments;
        uint32_t m_countColorAttach;

        EVK_NODISCARD bool IsReady() const noexcept { return m_countAttachments > 0 && m_self != VK_NULL_HANDLE; }

        operator VkRenderPass() const { return m_self; }
    };

    static void DestroyRenderPass(EvoVulkan::Types::Device *device, RenderPass *renderPass) {
        VK_LOG("Tools::DestroyRenderPass() : destroy vulkan render pass...");

        if (renderPass && renderPass->IsReady()) {
            vkDestroyRenderPass(*device, renderPass->m_self, nullptr);
            renderPass->m_self = VK_NULL_HANDLE;
            renderPass->m_countAttachments = 0;
            renderPass->m_countColorAttach = 0;
        }
        else {
            VK_ERROR("Tools::DestroyRenderPass() : render pass is nullptr!");
        }
    }

    static RenderPass CreateRenderPass(
            const EvoVulkan::Types::Device* device,
            const Types::Swapchain *swapchain,
            std::vector<VkAttachmentDescription> attachments,
            std::vector<VkAttachmentReference> inputAttachments,
            uint8_t sampleCount,
            VkImageAspectFlags depthAspect,
            VkFormat depthFormat
    ) {
        VK_GRAPH("Types::CreateRenderPass() : create vulkan render pass...");

        std::vector<VkAttachmentReference> colorReferences = {};
        std::vector<VkAttachmentReference> resolveReferences = {};
        VkAttachmentReference depthReference = {};
        /// Resolve attachment reference for the color attachment

        const bool multisampling = sampleCount > 1;
        const bool depth = depthAspect != EvoVulkan::Tools::Initializers::EVK_IMAGE_ASPECT_NONE;

        /// if (device->IsSeparateDepthStencilLayoutsSupported()) {
        ///     if ((depthAspect & VK_IMAGE_ASPECT_DEPTH_BIT) && (depthAspect & VK_IMAGE_ASPECT_STENCIL_BIT)) {
        ///         /// уже задали
        ///     }
        ///     else if (depthAspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
        ///         depthLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        ///     }
        ///     else if (depthAspect & VK_IMAGE_ASPECT_STENCIL_BIT) {
        ///         depthLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        ///     }
        /// }
        VkImageLayout depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if (attachments.empty()) {
            attachments.resize(multisampling ? (depth ? 3 : 2) : (depth ? 2 : 1));

            /// Color attachment
            attachments[0].format = swapchain->GetColorFormat();
            attachments[0].samples = Tools::Convert::IntToSampleCount(sampleCount);
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            if (multisampling) {
                /// This is the frame buffer attachment to where the multisampled image
                /// will be resolved to and which will be presented to the swapchain
                attachments[1].format = swapchain->GetColorFormat();
                attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                /// Multisampled depth attachment we render to
                if (depth) {
                    attachments[2].format = depthFormat;
                    attachments[2].samples = Tools::Convert::IntToSampleCount(sampleCount);
                    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachments[2].finalLayout = depthLayout;
                }

                resolveReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
            }
            else if (depth) {
                /// Depth attachment
                attachments[1].format = depthFormat;
                attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[1].finalLayout = depthLayout;
            }

            colorReferences.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            depthReference = { multisampling ? 2u : 1u, depthLayout};
        }
        else {
            ///uint32_t bind = 0;
            for (uint32_t i = 0; i < attachments.size() - (depth ? 1 : 0); i++) {
                colorReferences.push_back({i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                if (multisampling) {
                    VkAttachmentDescription attachmentDescription = {
                            .flags = 0,
                            .format = attachments[i].format,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            .initialLayout = attachments[i].initialLayout, //VK_IMAGE_LAYOUT_UNDEFINED,
                            .finalLayout = attachments[i].finalLayout //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    };

                    attachments.insert(attachments.begin() + i + 1, attachmentDescription);
                    i++;
                    resolveReferences.push_back(VkAttachmentReference{ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                }
            }

            if (depth) {
                depthReference = VkAttachmentReference{
                    static_cast<uint32_t>(colorReferences.size() + resolveReferences.size()),
                    depthLayout
                };
            }
        }

        VkSubpassDescription subpassDescription = { };
        {
            subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount    = static_cast<uint32_t>(colorReferences.size());
            subpassDescription.pColorAttachments       = colorReferences.data();
            subpassDescription.pDepthStencilAttachment = depth ? &depthReference : nullptr;
            subpassDescription.inputAttachmentCount    = inputAttachments.size();
            subpassDescription.pInputAttachments       = inputAttachments.data();
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments    = nullptr;
            subpassDescription.pResolveAttachments     = multisampling ? resolveReferences.data() : nullptr;
        }

        /// Subpass dependencies for layout transitions
        std::vector<VkSubpassDependency> dependencies;

        /// цвет с мультисемплингом
        if (multisampling) {
            dependencies.resize(2);

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }
        /// просто цвет без мультисемплинга
        else if (attachments.size() > 1 || !depth) {
            dependencies.resize(1);

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = 0;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        /// только буфер глубины без мультисемплинга
        else {
            dependencies.resize(2);

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[0].srcAccessMask = 0;
            dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }

        /// validate
        for (uint32_t i = 0; i < attachments.size(); ++i) {
            if (!Tools::IsFormatInRange(attachments[i].format)) {
                VK_HALT("RenderPass::CreateRenderPass() : format is in not a range! Index: " + std::to_string(i));
                return RenderPass(); /// NOLINT
            }
        }

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
            return RenderPass(); /// NOLINT
        }

        VK_LOG("Types::CreateRenderPass() : vulkan render pass " + EvoVulkan::Tools::PointerToString(renderPass) + " created successfully!");

        return { renderPass, (uint32_t)attachments.size(), (uint32_t)colorReferences.size() };
    }
}

#endif //EVOVULKAN_RENDERPASS_H
