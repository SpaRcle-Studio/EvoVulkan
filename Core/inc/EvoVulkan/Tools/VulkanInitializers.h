//
// Created by Nikita on 03.05.2021.
//

#ifndef EVOVULKAN_VULKANINITIALIZERS_H
#define EVOVULKAN_VULKANINITIALIZERS_H

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

namespace EvoVulkan::Tools::Initializers {
    EVK_MAYBE_UNUSED static VkMappedMemoryRange MappedMemoryRange(){
        VkMappedMemoryRange mappedMemoryRange = {};
        mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        return mappedMemoryRange;
    }

    EVK_MAYBE_UNUSED static VkRenderPassCreateInfo RenderPassCreateInfo() {
        VkRenderPassCreateInfo renderPassCreateInfo {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        return renderPassCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkViewport Viewport(
            float width,
            float height,
            float minDepth,
            float maxDepth)
    {
        VkViewport viewport {};
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;
        return viewport;
    }

    EVK_MAYBE_UNUSED static VkRect2D Rect2D(
            int32_t width,
            int32_t height,
            int32_t offsetX,
            int32_t offsetY)
    {
        VkRect2D rect2D {};
        rect2D.extent.width = width;
        rect2D.extent.height = height;
        rect2D.offset.x = offsetX;
        rect2D.offset.y = offsetY;
        return rect2D;
    }

    EVK_MAYBE_UNUSED static VkVertexInputBindingDescription VertexInputBindingDescription(
            uint32_t binding,
            uint32_t stride,
            VkVertexInputRate inputRate)
    {
        VkVertexInputBindingDescription vInputBindDescription {};
        vInputBindDescription.binding   = binding;
        vInputBindDescription.stride    = stride;
        vInputBindDescription.inputRate = inputRate;

        return vInputBindDescription;
    }

    EVK_MAYBE_UNUSED static VkVertexInputAttributeDescription VertexInputAttributeDescription(
            uint32_t binding,
            uint32_t location,
            VkFormat format,
            uint32_t offset)
    {
        VkVertexInputAttributeDescription vInputAttribDescription {};
        vInputAttribDescription.location = location;
        vInputAttribDescription.binding  = binding;
        vInputAttribDescription.format   = format;
        vInputAttribDescription.offset   = offset;

        return vInputAttribDescription;
    }

    EVK_MAYBE_UNUSED static VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage) {
        VkPipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage  = stage;
        shaderStage.module = shaderModule;
        shaderStage.pName  = "main";

        return shaderStage;
    }

    EVK_MAYBE_UNUSED static VkMemoryAllocateInfo MemoryAllocateInfo() {
        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        return memAllocInfo;
    }

    EVK_MAYBE_UNUSED static VkRenderPassBeginInfo RenderPassBeginInfo() {
        VkRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        return renderPassBeginInfo;
    }

    EVK_MAYBE_UNUSED static VkBufferCreateInfo BufferCreateInfo(
            VkBufferUsageFlags usage,
            VkDeviceSize size)
    {
        VkBufferCreateInfo bufCreateInfo = {};
        bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufCreateInfo.usage = usage;
        bufCreateInfo.size  = size;

        return bufCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
    {
        VkDescriptorImageInfo descriptorImageInfo {};
        descriptorImageInfo.sampler = sampler;
        descriptorImageInfo.imageView = imageView;
        descriptorImageInfo.imageLayout = imageLayout;
        return descriptorImageInfo;
    }

    EVK_MAYBE_UNUSED static VkImageCreateInfo ImageCreateInfo() {
        VkImageCreateInfo imageCreateInfo {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        return imageCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkWriteDescriptorSet WriteDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorImageInfo *imageInfo,
            uint32_t descriptorCount = 1)
    {
        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pImageInfo = imageInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;

        return writeDescriptorSet;
    }

    EVK_MAYBE_UNUSED static VkWriteDescriptorSet WriteDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorBufferInfo* bufferInfo,
            uint32_t descriptorCount = 1)
    {
        VK_ASSERT(binding != static_cast<uint32_t>(-1));

        if (bufferInfo->buffer == VK_NULL_HANDLE)
            VK_WARN("Initializers::WriteDescriptorSet() : buffer is NULL! You must create uniform buffer.");

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = dstSet;
        writeDescriptorSet.descriptorType  = type;
        writeDescriptorSet.dstBinding      = binding;
        writeDescriptorSet.pBufferInfo     = bufferInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;

        return writeDescriptorSet;
    }

    EVK_MAYBE_UNUSED static VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(
            uint32_t poolSizeCount,
            const VkDescriptorPoolSize* pPoolSizes,
            uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext         = NULL;
        descriptorPoolInfo.poolSizeCount = poolSizeCount;
        descriptorPoolInfo.pPoolSizes    = pPoolSizes;
        descriptorPoolInfo.maxSets       = maxSets;

        return descriptorPoolInfo;
    }

    EVK_MAYBE_UNUSED static VkDescriptorPoolSize DescriptorPoolSize(
            VkDescriptorType type,
            uint32_t descriptorCount)
    {
        VkDescriptorPoolSize descriptorPoolSize = {};
        descriptorPoolSize.type                 = type;
        descriptorPoolSize.descriptorCount      = descriptorCount;

        return descriptorPoolSize;
    }

    EVK_MAYBE_UNUSED static VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
            VkDescriptorType type,
            VkShaderStageFlags stageFlags,
            uint32_t binding,
            uint32_t descriptorCount = 1)
    {
        VkDescriptorSetLayoutBinding setLayoutBinding = {};
        setLayoutBinding.descriptorType  = type;
        setLayoutBinding.stageFlags      = stageFlags;
        setLayoutBinding.binding         = binding;
        setLayoutBinding.descriptorCount = descriptorCount;

        return setLayoutBinding;
    }

    EVK_MAYBE_UNUSED static VkImageViewCreateInfo ImageViewCreateInfo() {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        return image_view_create_info;
    }

    EVK_MAYBE_UNUSED static VkSamplerCreateInfo SamplerCreateInfo() {
        VkSamplerCreateInfo samplerCreateInfo {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.maxAnisotropy = 1.0f;
        return samplerCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(
            const VkDescriptorSetLayoutBinding* pBindings,
            uint32_t bindingCount)
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings    = pBindings;
        descriptorSetLayoutCreateInfo.bindingCount = bindingCount;

        return descriptorSetLayoutCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(
            VkDescriptorPool descriptorPool,
            const VkDescriptorSetLayout* pSetLayouts,
            uint32_t descriptorSetCount)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocateInfo.pSetLayouts        = pSetLayouts;
        descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;

        return descriptorSetAllocateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(
        const VkDescriptorSetLayout* pSetLayouts,
        uint32_t setLayoutCount,
        const std::vector<VkPushConstantRange>& pushConstants
    ) {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount         = setLayoutCount;
        pipelineLayoutCreateInfo.pSetLayouts            = pSetLayouts;
        pipelineLayoutCreateInfo.pPushConstantRanges    = pushConstants.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstants.size();

        return pipelineLayoutCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(
            const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
        descriptorSetLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings    = bindings.data();
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());

        return descriptorSetLayoutCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkSubmitInfo SubmitInfo() {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        return submitInfo;
    }

    EVK_MAYBE_UNUSED static VkSemaphoreCreateInfo SemaphoreCreateInfo() {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        return semaphoreCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0) {
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags             = flags;

        return fenceCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkCommandBufferAllocateInfo CommandBufferAllocateInfo(const VkCommandPool& commandPool, const VkCommandBufferLevel& level, uint32_t bufferCount) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool                 = commandPool;
        commandBufferAllocateInfo.level                       = level;
        commandBufferAllocateInfo.commandBufferCount          = bufferCount;

        return commandBufferAllocateInfo;
    }

    EVK_MAYBE_UNUSED static VkImageMemoryBarrier ImageMemoryBarrier() {
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = NULL;

        // Some default values
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        return imageMemoryBarrier;
    }

    EVK_MAYBE_UNUSED static VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo() {
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        return pipelineVertexInputStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo(
            const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
            const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
    )
    {
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
        return pipelineVertexInputStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(
            VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags,
            VkBool32 primitiveRestartEnable)
    {
        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
        pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineInputAssemblyStateCreateInfo.topology = topology;
        pipelineInputAssemblyStateCreateInfo.flags = flags;
        pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
        return pipelineInputAssemblyStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(
            VkPolygonMode polygonMode,
            VkCullModeFlags cullMode,
            VkFrontFace frontFace,
            VkPipelineRasterizationStateCreateFlags flags = 0)
    {
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
        pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
        pipelineRasterizationStateCreateInfo.cullMode = cullMode;
        pipelineRasterizationStateCreateInfo.frontFace = frontFace;
        pipelineRasterizationStateCreateInfo.flags = flags;
        pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

        ///pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_TRUE;
        ///pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 7.f;

        return pipelineRasterizationStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(
            VkColorComponentFlags colorWriteMask,
            VkBool32 blendEnable)
    {
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
        pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
        pipelineColorBlendAttachmentState.blendEnable = blendEnable;
        return pipelineColorBlendAttachmentState;
    }

    EVK_MAYBE_UNUSED static VkCommandBufferBeginInfo CommandBufferBeginInfo() {
        VkCommandBufferBeginInfo cmdBufferBeginInfo {};
        cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        return cmdBufferBeginInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(
            uint32_t attachmentCount,
            const VkPipelineColorBlendAttachmentState * pAttachments)
    {
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
        pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
        pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
        return pipelineColorBlendStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(
            VkBool32 depthTestEnable,
            VkBool32 depthWriteEnable,
            VkCompareOp depthCompareOp)
    {
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
        pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
        pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
        pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
        pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
        return pipelineDepthStencilStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(
            uint32_t viewportCount,
            uint32_t scissorCount,
            VkPipelineViewportStateCreateFlags flags = 0)
    {
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
        pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        pipelineViewportStateCreateInfo.viewportCount = viewportCount;
        pipelineViewportStateCreateInfo.scissorCount = scissorCount;
        pipelineViewportStateCreateInfo.flags = flags;
        return pipelineViewportStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples,
            VkPipelineMultisampleStateCreateFlags flags = 0)
    {
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
        pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
        pipelineMultisampleStateCreateInfo.flags = flags;
        pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        pipelineMultisampleStateCreateInfo.minSampleShading = 1.f;
        return pipelineMultisampleStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(
            const VkDynamicState * pDynamicStates,
            uint32_t dynamicStateCount,
            VkPipelineDynamicStateCreateFlags flags = 0)
    {
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo {};
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates;
        pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
        pipelineDynamicStateCreateInfo.flags = flags;
        return pipelineDynamicStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(
            const std::vector<VkDynamicState>& pDynamicStates,
            VkPipelineDynamicStateCreateFlags flags = 0)
    {
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
        pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
        pipelineDynamicStateCreateInfo.flags = flags;
        return pipelineDynamicStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkPipelineTessellationStateCreateInfo PipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
    {
        VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo {};
        pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        pipelineTessellationStateCreateInfo.patchControlPoints = patchControlPoints;
        return pipelineTessellationStateCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkGraphicsPipelineCreateInfo PipelineCreateInfo(
            VkPipelineLayout layout,
            VkRenderPass renderPass,
            VkPipelineCreateFlags flags = 0)
    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.flags = flags;
        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkGraphicsPipelineCreateInfo PipelineCreateInfo()
    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        return pipelineCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkComputePipelineCreateInfo ComputePipelineCreateInfo(
            VkPipelineLayout layout,
            VkPipelineCreateFlags flags = 0)
    {
        VkComputePipelineCreateInfo computePipelineCreateInfo {};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.layout = layout;
        computePipelineCreateInfo.flags = flags;
        return computePipelineCreateInfo;
    }

    EVK_MAYBE_UNUSED static VkFramebufferCreateInfo FrameBufferCI(VkRenderPass renderPass, uint32_t width, uint32_t height) {
        auto info = VkFramebufferCreateInfo();

        info.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.width      = width;
        info.height     = height;
        info.layers     = 1;
        info.renderPass = renderPass;

        return info;
    }
}

#endif //EVOVULKAN_VULKANINITIALIZERS_H
