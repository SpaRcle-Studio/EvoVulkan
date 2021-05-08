//
// Created by Nikita on 08.05.2021.
//

#include <EvoVulkan/Complexes/Shader.h>
#include <fstream>

#include <EvoVulkan/Tools/StringUtils.h>
#include <EvoVulkan/Tools/VulkanTools.h>

EvoVulkan::Complexes::Shader::Shader(
        const EvoVulkan::Types::Device* device,
        const VkRenderPass& renderPass,
        const VkPipelineCache& cache)
{
    this->m_device     = device;
    this->m_renderPass = renderPass;
    this->m_cache      = cache;
}

bool EvoVulkan::Complexes::Shader::Load(
        const std::string& source, const std::string& cache,
        const std::vector<std::pair<const char *, VkShaderStageFlagBits>> &modules,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
        const std::vector<VkDeviceSize>& uniformSizes)
{
    this->m_uniformSizes   = uniformSizes;
    this->m_layoutBindings = descriptorLayoutBindings;

    for (auto module : modules) {
        std::string out = cache + "/" + std::string(module.first) + ".spv";

        if (file_exists(out))
            std::remove(std::string("del " + out).c_str());

        system(std::string("glslc -c " + source + "/" + std::string(module.first) +" -o " + out).c_str());

        auto shaderModule = Tools::LoadShaderModule(out.c_str(), *m_device);
        if (shaderModule == VK_NULL_HANDLE) {
            VK_ERROR("Shader::Load() : failed to load shader module! \n\tPath: " + out);
            return false;
        }
        else {
            this->m_shaderModules.push_back(shaderModule);
            this->m_shaderStages.push_back(
                    Tools::Initializers::PipelineShaderStageCreateInfo(shaderModule, module.second));
        }
    }

    return true;
}

bool EvoVulkan::Complexes::Shader::SetVertexDescriptions(
        const std::vector<VkVertexInputBindingDescription> &binding,
        const std::vector<VkVertexInputAttributeDescription> &attribute)
{
    this->m_vertices.m_bindingDescriptions   = binding;
    this->m_vertices.m_attributeDescriptions = attribute;

    m_vertices.m_inputState = Tools::Initializers::PipelineVertexInputStateCreateInfo();
    m_vertices.m_inputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_vertices.m_bindingDescriptions.size());
    m_vertices.m_inputState.pVertexBindingDescriptions      = m_vertices.m_bindingDescriptions.data();
    m_vertices.m_inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertices.m_attributeDescriptions.size());
    m_vertices.m_inputState.pVertexAttributeDescriptions    = m_vertices.m_attributeDescriptions.data();

    return true;
}

bool EvoVulkan::Complexes::Shader::Compile(
        VkPolygonMode polygonMode,
        VkCullModeFlags cullMode,
        VkCompareOp depthCompare,
        VkBool32 blendEnable,
        VkBool32 depthEnable)
{
    if (!this->BuildLayouts()) {
        VK_ERROR("Shader::Compile() : failed to build layouts!");
        return false;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Tools::Initializers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = Tools::Initializers::PipelineRasterizationStateCreateInfo(polygonMode, cullMode, VK_FRONT_FACE_CLOCKWISE, 0);
    VkPipelineColorBlendAttachmentState    blendAttachmentState = Tools::Initializers::PipelineColorBlendAttachmentState(0xf, blendEnable);
    VkPipelineColorBlendStateCreateInfo    colorBlendState = Tools::Initializers::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo  depthStencilState = Tools::Initializers::PipelineDepthStencilStateCreateInfo(depthEnable, VK_TRUE, depthCompare);
    VkPipelineViewportStateCreateInfo      viewportState = Tools::Initializers::PipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo   multisampleState = Tools::Initializers::PipelineMultisampleStateCreateInfo(m_device->GetMSAASamples(), 0);

    std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState =
            Tools::Initializers::PipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
            Tools::Initializers::PipelineCreateInfo(m_pipelineLayout, m_renderPass, 0);

    pipelineCreateInfo.pVertexInputState   = &m_vertices.m_inputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pMultisampleState   = &multisampleState;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.stageCount          = static_cast<uint32_t>(m_shaderStages.size());
    pipelineCreateInfo.pStages             = m_shaderStages.data();

    if (vkCreateGraphicsPipelines(*m_device, m_cache, 1, &pipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        VK_ERROR("Shader::Compile() : failed to create vulkan graphics pipeline!");
        return false;
    }

    return true;
}

bool EvoVulkan::Complexes::Shader::BuildLayouts() {
    this->m_descriptorSetLayout = Tools::CreateDescriptorLayout(*m_device, m_layoutBindings);
    if (this->m_descriptorSetLayout == VK_NULL_HANDLE) {
        VK_ERROR("Shader::BuildLayouts() : failed to create descriptor layout!");
        return false;
    }

    this->m_pipelineLayout = Tools::CreatePipelineLayout(*m_device, m_descriptorSetLayout);
    if (this->m_pipelineLayout == VK_NULL_HANDLE) {
        VK_ERROR("Shader::BuildLayouts() : failed to create pipeline layout!");
        return false;
    }

    return true;
}

void EvoVulkan::Complexes::Shader::Destroy() {
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(*m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(*m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    for (auto module : m_shaderModules) {
        vkDestroyShaderModule(*m_device, module, nullptr);
        m_shaderModules.clear();
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(*m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    this->m_cache = VK_NULL_HANDLE;
}

void EvoVulkan::Complexes::Shader::Free() {
    delete this;
}

/*
bool EvoVulkan::Complex::Shader::AutoUniformDetection(
        const std::string &source,
        const std::vector<std::pair<const char *, VkShaderStageFlagBits>> &modules)
{
    std::map<VkShaderStageFlagBits, std::vector<VkDescriptorType>> uniforms = {};

    for (std::pair<const char*, VkShaderStageFlagBits> module : modules) {
        std::string path = source + "/" + std::string(module.first);

        std::ifstream is(path);
        if (!is.is_open()) {
            VK_ERROR("Shader::AutoUniformDetection() : could not open file! \nPath: " + path);
            return false;
        } else {
            std::string line;
            while (!is.eof()) {
                std::getline(is, line);

                if (Tools::Contains(line, "uniform")) {
                    VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    std::cout << line << std::endl;
                }
            }
        }
    }
}*/
