//
// Created by Nikita on 08.05.2021.
//

#include <EvoVulkan/Complexes/Shader.h>
#include <fstream>

#include <EvoVulkan/Tools/StringUtils.h>
#include <EvoVulkan/Tools/VulkanTools.h>

EvoVulkan::Complexes::Shader::Shader(
        const EvoVulkan::Types::Device* device,
        Types::RenderPass renderPass,
        const VkPipelineCache& cache)
{
    this->m_device     = device;
    this->m_renderPass = renderPass;
    this->m_cache      = cache;
}

bool EvoVulkan::Complexes::Shader::Load(
        const std::string& source, const std::string& cache,
        const std::vector<std::pair<std::string, VkShaderStageFlagBits>> &modules,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
        const std::vector<VkDeviceSize>& uniformSizes)
{
    if (!this) {
        VK_ERROR("Shader::Load() : WTF!? this is nullptr!");
        return false;
    }

    auto modules_names = std::string();
    for (const auto & module : modules)
        modules_names += std::string(module.first).append(" ");
    VK_LOG("Shader::Load() : load new shader...\n\tSource: " + source + "\n\tModules: " + modules_names);

    // check correctly uniform sizes
    {
        uint32_t count = 0;
        for (auto bind : descriptorLayoutBindings)
            if (bind.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                count++;
        if (count != uniformSizes.size()) {
            VK_ERROR("Shader::Load() : incorrect uniform sizes!");
            return false;
        }
    }

    if (!uniformSizes.empty())
        this->m_uniformSizes   = uniformSizes;

    this->m_layoutBindings = descriptorLayoutBindings;
    for (size_t t = 0; t < m_layoutBindings.size(); t++)
        if (t != m_layoutBindings[t].binding) {
            VK_ERROR("Shader::Load() : incorrect layout bindings! Binding: " + std::to_string(t));
            return false;
        }

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
    for (uint32_t i = 0; i < binding.size(); i++)
        if (binding[i].binding != i || binding[i].stride <= 0) {
            VK_ERROR("Shader::SetVertexDescriptions() : incorrect vertex binding!");
            return false;
        }

    this->m_vertices.m_bindingDescriptions   = binding;
    this->m_vertices.m_attributeDescriptions = attribute;

    m_vertices.m_inputState = Tools::Initializers::PipelineVertexInputStateCreateInfo();
    m_vertices.m_inputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_vertices.m_bindingDescriptions.size());
    m_vertices.m_inputState.pVertexBindingDescriptions      = m_vertices.m_bindingDescriptions.data();
    m_vertices.m_inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertices.m_attributeDescriptions.size());
    m_vertices.m_inputState.pVertexAttributeDescriptions    = m_vertices.m_attributeDescriptions.data();

    this->m_hasVertices = true;

    return true;
}

bool EvoVulkan::Complexes::Shader::Compile(
        VkPolygonMode polygonMode,
        VkCullModeFlags cullMode,
        VkCompareOp depthCompare,
        VkBool32 blendEnable,
        VkBool32 depthEnable) //, uint32_t countAttachments
{
    if (!this->BuildLayouts()) {
        VK_ERROR("Shader::Compile() : failed to build layouts!");
        return false;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = Tools::Initializers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = Tools::Initializers::PipelineRasterizationStateCreateInfo(polygonMode, cullMode, VK_FRONT_FACE_CLOCKWISE, 0);

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {};

    for (uint32_t i = 0; i < m_renderPass.m_countColorAttach; i++) {
        auto writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        auto attch = Tools::Initializers::PipelineColorBlendAttachmentState(writeMask, blendEnable);

        attch.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attch.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        attch.colorBlendOp        = VK_BLEND_OP_ADD;

        //attch.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attch.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        //attch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        blendAttachmentStates.push_back(attch);


        //blendAttachmentStates.push_back(Tools::Initializers::PipelineColorBlendAttachmentState(0xf, blendEnable));
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState =
            Tools::Initializers::PipelineColorBlendStateCreateInfo(m_renderPass.m_countColorAttach, blendAttachmentStates.data());

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
            Tools::Initializers::PipelineCreateInfo(m_pipelineLayout, m_renderPass.m_self, 0);

    if (!m_hasVertices)
        m_vertices.m_inputState = Tools::Initializers::PipelineVertexInputStateCreateInfo();

    pipelineCreateInfo.pVertexInputState = &m_vertices.m_inputState;

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
