//
// Created by Nikita on 05.05.2021.
//

#include <EvoVulkan/Tools/VulkanTools.h>

namespace EvoVulkan::Tools {
    VkShaderModule LoadShaderModule(const std::vector<uint32_t>& spirV, VkDevice device) {
        if (spirV.empty()) {
            VK_ERROR("Tools::LoadShaderModule() : spirV is empty!");
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spirV.size() * sizeof(uint32_t);
        createInfo.pCode = spirV.data();

        VkShaderModule module;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
            VK_ERROR("Tools::LoadShaderModule() : failed to create vulkan shader module from spirV!");
            return VK_NULL_HANDLE;
        }
        return module;
    }

    VkShaderModule LoadShaderModule(const char *fileName, VkDevice device) {
        std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

        if (is.is_open()) {
            size_t size = is.tellg();
            is.seekg(0, std::ios::beg);
            char* shaderCode = new char[size];
            is.read(shaderCode, size);
            is.close();

            if (size <= 0) {
                VK_ERROR("Tools::LoadShaderModule() : failed to read shader! \nPath: " + std::string(fileName));
                return VK_NULL_HANDLE;
            }

            VkShaderModule shaderModule;
            VkShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = size;
            moduleCreateInfo.pCode = (uint32_t*)shaderCode;

            auto result = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule);
            if (result != VK_SUCCESS) {
                VK_ERROR("Tools::LoadShaderModule() : failed to create vulkan shader module! \nPath: " + std::string(fileName));
                return VK_NULL_HANDLE;
            }

            delete[] shaderCode;

            return shaderModule;
        }
        else {
            VK_ERROR("Tools::LoadShaderModule() : could not open shader file! \n\tPath: " + std::string(fileName));

            return VK_NULL_HANDLE;
        }
    }

    VkPipelineLayout CreatePipelineLayout(const VkDevice& device, uint32_t setLayoutCount, VkDescriptorSetLayout descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstants) {
        for (auto&& pushConstant : pushConstants) {
            if (pushConstant.stageFlags == 0) {
                VK_ERROR("Tools::CreatePipelineLayout() : push constant does not contains any stages!");
                return VK_NULL_HANDLE;
            }
        }

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = Initializers::PipelineLayoutCreateInfo(&descriptorSetLayout, setLayoutCount, pushConstants);

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        auto result = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);

        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreatePipelineLayout() : failed to create pipeline layout!");
            return VK_NULL_HANDLE;
        }
        else
            return pipelineLayout;
    }

    VkDescriptorSetLayout CreateDescriptorSetLayout(const VkDevice& device, const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings) {
        VkDescriptorSetLayoutCreateInfo descriptorLayout = Initializers::DescriptorSetLayoutCreateInfo(setLayoutBindings);

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

        auto result = vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateDescriptorSetLayout() : failed to create descriptor set layout!");
            return VK_NULL_HANDLE;
        }
        else
            return descriptorSetLayout;
    }

    VkPipeline CreateStandardGeometryPipeLine(
            const Types::Device* device,
            const VkRenderPass& renderPass,
            const VkPipelineLayout& pipelineLayout,
            const std::vector<VkDynamicState>& dynamicStateEnables,
            std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
            VkVertexInputBindingDescription vertexInputBinding,
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
            VkPipelineCache pipelineCache)
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
                Initializers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState =
                Initializers::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        VkPipelineColorBlendAttachmentState    blendAttachmentState =
                Initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo    colorBlendState =
                Initializers::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo  depthStencilState =
                Initializers::PipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo      viewportState =
                Initializers::PipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo   multisampleState =
                Initializers::PipelineMultisampleStateCreateInfo(device->GetMSAASamples(), 0);
        VkPipelineDynamicStateCreateInfo       dynamicState =
                Initializers::PipelineDynamicStateCreateInfo(dynamicStateEnables);

        VkGraphicsPipelineCreateInfo pipelineCI = Initializers::PipelineCreateInfo(pipelineLayout, renderPass, 0);

        return VK_NULL_HANDLE;
    }

    void DestroyPipelineCache(const VkDevice& device, VkPipelineCache* cache) {
        if (!cache || *cache == VK_NULL_HANDLE) {
            VK_ERROR("Tools::DestroyPipelineCache() : cache is nullptr!");
            return;
        }

        vkDestroyPipelineCache(device, *cache, nullptr);
        *cache = VK_NULL_HANDLE;
    }

    VkPipelineCache CreatePipelineCache(const VkDevice& device) {
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        auto result = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);

        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreatePipelineCache() : failed to create pipeline cache! Reason:" +
                     Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return pipelineCache;
    }

    VkSemaphore CreateVulkanSemaphore(const VkDevice& device) {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkSemaphoreCreateInfo semaphoreCreateInfo = Initializers::SemaphoreCreateInfo();
        auto result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateVulkanSemaphore() : failed to create vulkan semaphore!");
            return VK_NULL_HANDLE;
        }
        return semaphore;
    }

    void DestroyVulkanSemaphore(const VkDevice& device, VkSemaphore* semaphore) {
        if (semaphore && *semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, *semaphore, nullptr);
            *semaphore = VK_NULL_HANDLE;
        }
    }

    void DestroySynchronization(const VkDevice& device, Types::Synchronization* sync) {
        VK_LOG("Tools::DestroySynchronization() : destroy vulkan synchronizations...");

        if (!sync->IsReady()) {
            VK_ERROR("Tools::DestroySynchronization() : synchronizations isn't ready!");
            return;
        }

        vkDestroySemaphore(device, sync->m_presentComplete, nullptr);
        vkDestroySemaphore(device, sync->m_renderComplete, nullptr);

        sync->m_presentComplete = VK_NULL_HANDLE;
        sync->m_renderComplete  = VK_NULL_HANDLE;
    }

    Types::Synchronization CreateSynchronization(const VkDevice& device) {
        VK_GRAPH("Tools::CreateSynchronization() : create vulkan synchronizations...");

        Types::Synchronization sync = {};

        // Create synchronization objects
        VkSemaphoreCreateInfo semaphoreCreateInfo = Initializers::SemaphoreCreateInfo();
        // Create a semaphore used to synchronize image presentation
        // Ensures that the image is displayed before we start submitting new commands to the queue
        auto result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sync.m_presentComplete);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateSynchronization() : failed to create present semaphore!");
            return {};
        }
        // Create a semaphore used to synchronize command submission
        // Ensures that the image is not presented until all commands have been submitted and executed
        result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &sync.m_renderComplete);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateSynchronization() : failed to create render semaphore!");
            return {};
        }

        return sync;
    }

    VkAttachmentDescription2 CreateColorAttachmentDescription(
            VkFormat format,
            VkSampleCountFlagBits samples,
            VkImageLayout init,
            VkImageLayout final)
    {
        VkAttachmentDescription2 attachmentDescription = {};
        attachmentDescription.sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachmentDescription.pNext          = nullptr;
        attachmentDescription.flags          = 0;
        attachmentDescription.format         = format;
        attachmentDescription.samples        = samples;
        attachmentDescription.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD, //! clear or load?
        attachmentDescription.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout  = init;
        attachmentDescription.finalLayout    = final;
        return attachmentDescription;
    }

    VkDescriptorSetLayout CreateDescriptorLayout(VkDevice const &device, const std::vector<VkDescriptorSetLayoutBinding> &setLayoutBindings) {
        auto descriptorSetLayoutCreateInfo = Initializers::DescriptorSetLayoutCreateInfo(
                setLayoutBindings.data(),
                static_cast<uint32_t>(setLayoutBindings.size()));

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        auto result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateDescriptorLayout() : failed to create descriptor set layout!");
            return VK_NULL_HANDLE;
        }
        else
            return descriptorSetLayout;
    }
}