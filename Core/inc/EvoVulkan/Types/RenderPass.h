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

    static VkRenderPass CreateOldRenderPass(
        const EvoVulkan::Types::Device* device,
        std::vector<VkAttachmentDescription2> attachments,
        std::vector<VkAttachmentReference2> inputAttachments,
        std::vector<VkAttachmentReference2> colorReferences,
        std::vector<VkAttachmentReference2> resolveReferences,
        VkAttachmentReference2 depthReference,
        std::vector<VkSubpassDependency2> dependencies,
        bool multisampling,
        bool depth,
        VkSubpassDescription2 subpassDescription
    ) {
        std::vector<VkSubpassDependency> dependenciesOld;
        for (auto& dep : dependencies) {
            VkSubpassDependency depOld = {};
            depOld.srcSubpass = dep.srcSubpass;
            depOld.dstSubpass = dep.dstSubpass;
            depOld.srcStageMask = dep.srcStageMask;
            depOld.dstStageMask = dep.dstStageMask;
            depOld.srcAccessMask = dep.srcAccessMask;
            depOld.dstAccessMask = dep.dstAccessMask;
            depOld.dependencyFlags = dep.dependencyFlags;
            dependenciesOld.push_back(depOld);
        }

        std::vector<VkAttachmentReference> inputAttachmentsOld;
        for (auto& input : inputAttachments) {
            VkAttachmentReference inputOld = {};
            inputOld.attachment = input.attachment;
            inputOld.layout = input.layout;
            inputAttachmentsOld.push_back(inputOld);
        }

        std::vector<VkAttachmentReference> colorReferencesOld;
        for (auto& color : colorReferences) {
            VkAttachmentReference colorOld = {};
            colorOld.attachment = color.attachment;
            colorOld.layout = color.layout;
            colorReferencesOld.push_back(colorOld);
        }

        std::vector<VkAttachmentReference> resolveReferencesOld;
        for (auto& resolve : resolveReferences) {
            VkAttachmentReference resolveOld = {};
            resolveOld.attachment = resolve.attachment;
            resolveOld.layout = resolve.layout;
            resolveReferencesOld.push_back(resolveOld);
        }

        VkAttachmentReference depthReferenceOld = {};
        depthReferenceOld.attachment = depthReference.attachment;
        depthReferenceOld.layout = depthReference.layout;

        VkSubpassDescription subpassDescriptionOld = {};
        subpassDescriptionOld.pipelineBindPoint = subpassDescription.pipelineBindPoint;
        subpassDescriptionOld.colorAttachmentCount = subpassDescription.colorAttachmentCount;
        subpassDescriptionOld.pColorAttachments = (subpassDescription.colorAttachmentCount > 0) ? colorReferencesOld.data() : nullptr;
        subpassDescriptionOld.pDepthStencilAttachment = depth ? &depthReferenceOld : nullptr;
        subpassDescriptionOld.inputAttachmentCount = subpassDescription.inputAttachmentCount;
        subpassDescriptionOld.pInputAttachments = (subpassDescription.inputAttachmentCount > 0) ? inputAttachmentsOld.data() : nullptr;
        subpassDescriptionOld.preserveAttachmentCount = 0;
        subpassDescriptionOld.pPreserveAttachments = nullptr;
        subpassDescriptionOld.pResolveAttachments = multisampling ? resolveReferencesOld.data() : nullptr;

        std::vector<VkAttachmentDescription> attachmentsOld;
        for (auto& att : attachments) {
            VkAttachmentDescription attOld = {};
            attOld.flags = att.flags;
            attOld.format = att.format;
            attOld.samples = att.samples;
            attOld.loadOp = att.loadOp;
            attOld.storeOp = att.storeOp;
            attOld.stencilLoadOp = att.stencilLoadOp;
            attOld.stencilStoreOp = att.stencilStoreOp;
            attOld.initialLayout = att.initialLayout;
            attOld.finalLayout = att.finalLayout;
            attachmentsOld.push_back(attOld);
        }

        VkRenderPassCreateInfo renderPassInfoOld = {};
        renderPassInfoOld.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfoOld.pNext = nullptr;
        renderPassInfoOld.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfoOld.pAttachments = attachmentsOld.data();
        renderPassInfoOld.subpassCount = 1;
        renderPassInfoOld.pSubpasses = &subpassDescriptionOld;
        renderPassInfoOld.dependencyCount = static_cast<uint32_t>(dependenciesOld.size());
        renderPassInfoOld.pDependencies = dependenciesOld.data();
        VkRenderPass renderPass = VK_NULL_HANDLE;
        auto result = vkCreateRenderPass(*device, &renderPassInfoOld, nullptr, &renderPass);
        if (result != VK_SUCCESS) {
            VK_ERROR("Types::CreateOldRenderPass() : failed to create vulkan render pass! Reason: " + Tools::Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }
        return renderPass;
    }

    static RenderPass CreateRenderPass(
            const EvoVulkan::Types::Device* device,
            const Types::Swapchain *swapchain,
            std::vector<VkAttachmentDescription2> attachments,
            std::vector<VkAttachmentReference2> inputAttachments,
            uint8_t sampleCount,
            VkImageAspectFlags depthAspect,
            VkFormat depthFormat
    ) {
        VK_GRAPH("Types::CreateRenderPass() : create vulkan render pass...");

        std::vector<VkAttachmentReference2> colorReferences = {};
        std::vector<VkAttachmentReference2> resolveReferences = {};
        VkAttachmentReference2 depthReference = {};
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

        const bool isEmpty = attachments.empty();
        if (isEmpty) {
            attachments.resize(multisampling ? (depth ? 3 : 2) : (depth ? 2 : 1));

            /// Color attachment
            attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            attachments[0].pNext = nullptr;
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
                attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
                attachments[1].pNext = nullptr;
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
                    attachments[2].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
                    attachments[2].pNext = nullptr;
                    attachments[2].format = depthFormat;
                    attachments[2].samples = Tools::Convert::IntToSampleCount(sampleCount);
                    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    attachments[2].finalLayout = depthLayout;
                }

                auto& ref = resolveReferences.emplace_back();
                ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                ref.pNext = nullptr;
                ref.attachment = 1;
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            else if (depth) {
                /// Depth attachment
                attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
                attachments[1].pNext = nullptr;
                attachments[1].format = depthFormat;
                attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachments[1].finalLayout = depthLayout;
            }

            auto& ref = colorReferences.emplace_back();
            ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            ref.pNext = nullptr;
            ref.attachment = 0;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            depthReference = VkAttachmentReference2();
            depthReference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            depthReference.pNext = nullptr;
            depthReference.layout = depthLayout;
            depthReference.attachment = multisampling ? 2u : 1u;
        }
        else {
            ///uint32_t bind = 0;
            for (uint32_t i = 0; i < attachments.size() - (depth ? 1 : 0); i++) {
                {
                    auto &ref = colorReferences.emplace_back();
                    ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    ref.pNext = nullptr;
                    ref.attachment = i;
                    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                if (multisampling) {
                    VkAttachmentDescription2 attachmentDescription = {
                        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                        .pNext = nullptr,
                        .flags = 0,
                        .format = attachments[i].format,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        //.samples = Tools::Convert::IntToSampleCount(sampleCount),
                        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout = attachments[i].initialLayout, //VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = attachments[i].finalLayout //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    };

                    attachments.insert(attachments.begin() + i + 1, attachmentDescription);
                    i++;

                    auto& ref = resolveReferences.emplace_back();
                    ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    ref.pNext = nullptr;
                    ref.attachment = i;
                    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }

            if (depth) {
                depthReference = VkAttachmentReference2();
                depthReference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                depthReference.pNext = nullptr;
                depthReference.attachment = static_cast<uint32_t>(colorReferences.size() + resolveReferences.size());
                depthReference.layout = depthLayout;
            }
        }

        VkAttachmentReference2 depthResolveAttachment{};
        depthResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        depthResolveAttachment.attachment = depthReference.attachment;
        depthResolveAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescriptionDepthStencilResolve depthResolve{};
        depthResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
        depthResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;// VK_RESOLVE_MODE_AVERAGE_BIT;   // или SAMPLE_ZERO_BIT
        depthResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE;
        depthResolve.pDepthStencilResolveAttachment = &depthResolveAttachment;

        VkSubpassDescription2KHR subpassDescription = { };
        {
            subpassDescription.sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
            subpassDescription.pNext                   = nullptr;
            //subpassDescription.pNext                   = (depth && !isEmpty && sampleCount > 1) ? &depthResolve : nullptr;
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
        std::vector<VkSubpassDependency2> dependencies;

        /// цвет с мультисемплингом
        if (multisampling) {
            dependencies.resize(2);

            dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[0].pNext = nullptr;
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[1].pNext = nullptr;
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

            dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[0].pNext = nullptr;
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

            dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[0].pNext = nullptr;
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependencies[0].srcAccessMask = 0;
            dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[1].pNext = nullptr;
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


        VkRenderPassCreateInfo2KHR renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR;
        renderPassInfo.pNext = nullptr;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        auto pVkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR) vkGetDeviceProcAddr(*device, "vkCreateRenderPass2KHR");
        if (!pVkCreateRenderPass2KHR) {
            pVkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR) vkGetDeviceProcAddr(*device, "vkCreateRenderPass2");
        }

        VkRenderPass renderPass = VK_NULL_HANDLE;

        if (!pVkCreateRenderPass2KHR) {
            static bool once = false;
            if (!once) {
                once = true;
                VK_LOG("Types::CreateRenderPass() : failed to get vkCreateRenderPass2KHR function pointer! Trying to use vkCreateRenderPass...");
            }

            renderPass = CreateOldRenderPass(device, attachments, inputAttachments, colorReferences, resolveReferences, depthReference, dependencies, multisampling, depth, subpassDescription);
            if (!renderPass) {
                VK_ERROR("Types::CreateRenderPass() : failed to create vulkan render pass using vkCreateRenderPass!");
                return RenderPass(); /// NOLINT
            }
        }
        else {
            auto result = pVkCreateRenderPass2KHR(*device, &renderPassInfo, nullptr, &renderPass);
            if (result != VK_SUCCESS) {
                VK_ERROR("Types::CreateRenderPass() : failed to create vulkan render pass! Reason: " + Tools::Convert::result_to_description(result));
                return RenderPass(); /// NOLINT
            }
        }

        VK_LOG("Types::CreateRenderPass() : vulkan render pass " + EvoVulkan::Tools::PointerToString(renderPass) + " created successfully!");

        return { renderPass, (uint32_t)attachments.size(), (uint32_t)colorReferences.size() };
    }

}

#endif //EVOVULKAN_RENDERPASS_H
