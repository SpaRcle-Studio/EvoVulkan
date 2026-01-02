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
            // Check if depth resolve attachment exists before processing
            // We need to count color attachments first to find where depth attachment starts
            bool hasDepthResolveInAttachments = false;
            if (depth && multisampling) {
                // Count how many color attachments we have (including their resolve attachments)
                // Color attachments come in pairs when multisampling: color, resolve, color, resolve, ...
                // So we need to find the first depth format attachment
                uint32_t depthAttachmentIndex = UINT32_MAX;
                for (uint32_t j = 0; j < attachments.size(); ++j) {
                    if (attachments[j].format == depthFormat && 
                        attachments[j].samples != VK_SAMPLE_COUNT_1_BIT) {
                        depthAttachmentIndex = j;
                        break;
                    }
                }
                
                // Check if there's a depth resolve attachment after the depth attachment
                if (depthAttachmentIndex != UINT32_MAX && depthAttachmentIndex + 1 < attachments.size()) {
                    const auto& potentialResolve = attachments[depthAttachmentIndex + 1];
                    if (potentialResolve.format == depthFormat && 
                        potentialResolve.samples == VK_SAMPLE_COUNT_1_BIT) {
                        hasDepthResolveInAttachments = true;
                    }
                }
            }
            
            ///uint32_t bind = 0;
            uint32_t attachmentsToProcess = attachments.size() - (depth ? (hasDepthResolveInAttachments ? 2 : 1) : 0);
            for (uint32_t i = 0; i < attachmentsToProcess; i++) {
                {
                    auto &ref = colorReferences.emplace_back();
                    ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    ref.pNext = nullptr;
                    ref.attachment = i;
                    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                if (multisampling) {
                    // Check if resolve attachment already exists (next attachment with same format and VK_SAMPLE_COUNT_1_BIT)
                    // Also check that it's not a depth attachment (depth format is different)
                    bool resolveExists = false;
                    if (i + 1 < attachments.size()) {
                        const auto& nextAtt = attachments[i + 1];
                        // Check if it's a color resolve attachment (same format as current, VK_SAMPLE_COUNT_1_BIT, and not a depth format)
                        if (nextAtt.format == attachments[i].format && 
                            nextAtt.samples == VK_SAMPLE_COUNT_1_BIT &&
                            attachments[i].samples != VK_SAMPLE_COUNT_1_BIT &&
                            nextAtt.format != depthFormat) { // Make sure it's not a depth resolve attachment
                            resolveExists = true;
                        }
                    }
                    
                    if (!resolveExists) {
                        VkAttachmentDescription2 attachmentDescription = {
                            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                            .pNext = nullptr,
                            .flags = 0,
                            .format = attachments[i].format,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                            .initialLayout = attachments[i].initialLayout,
                            .finalLayout = attachments[i].finalLayout
                        };

                        attachments.insert(attachments.begin() + i + 1, attachmentDescription);
                        i++;
                    }
                    else {
                        i++; // Skip the existing resolve attachment
                    }

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

        // Check if depth resolve attachment exists (it should be the attachment after depth with same format and VK_SAMPLE_COUNT_1_BIT)
        bool hasDepthResolve = false;
        uint32_t depthResolveAttachmentIndex = 0;
        if (depth && multisampling && !isEmpty) {
            uint32_t depthAttachmentIndex = static_cast<uint32_t>(colorReferences.size() + resolveReferences.size());
            if (depthAttachmentIndex + 1 < attachments.size()) {
                const auto& potentialResolve = attachments[depthAttachmentIndex + 1];
                if (potentialResolve.format == depthFormat && 
                    potentialResolve.samples == VK_SAMPLE_COUNT_1_BIT) {
                    hasDepthResolve = true;
                    depthResolveAttachmentIndex = depthAttachmentIndex + 1;
                }
            }
        }
        else if (depth && multisampling && isEmpty) {
            // For swapchain case, depth resolve would be at index 3 if depth is at index 2
            // But this case is not currently supported in the swapchain path
        }

        VkAttachmentReference2 depthResolveAttachmentRef{};
        VkSubpassDescriptionDepthStencilResolve depthResolve{};
        if (hasDepthResolve) {
            depthResolveAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            depthResolveAttachmentRef.pNext = nullptr;
            depthResolveAttachmentRef.attachment = depthResolveAttachmentIndex;
            depthResolveAttachmentRef.layout = Tools::FindDepthFormatLayout(depthAspect, true, false);

            depthResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
            depthResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
            depthResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE;
            depthResolve.pDepthStencilResolveAttachment = &depthResolveAttachmentRef;
        }

        VkSubpassDescription2KHR subpassDescription = { };
        {
            subpassDescription.sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR;
            subpassDescription.pNext                   = hasDepthResolve ? &depthResolve : nullptr;
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
            if (depth) {
                dependencies.resize(4);
            }
            else {
                dependencies.resize(2);
            }

            dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[0].pNext = nullptr;
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = 0;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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

            if (depth) {
                dependencies[2].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
                dependencies[2].pNext = nullptr;
                dependencies[2].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[2].dstSubpass = 0;
                dependencies[2].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dependencies[2].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[2].srcAccessMask = 0;
                dependencies[2].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[3].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
                dependencies[3].pNext = nullptr;
                dependencies[3].srcSubpass = 0;
                dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[3].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependencies[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }
        /// просто цвет без мультисемплинга
        else if (attachments.size() > 1 || !depth) {
            dependencies.resize(depth ? 2 : 1);

            dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[0].pNext = nullptr;
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = 0;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            if (depth) {
                dependencies[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
                dependencies[1].pNext = nullptr;
                dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[1].dstSubpass = 0;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[1].srcAccessMask = 0;
                dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            }
        }
        /// только буфер глубины без мультисемплинга
        else {
            dependencies.resize(2);

            dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dependencies[0].pNext = nullptr;
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
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
