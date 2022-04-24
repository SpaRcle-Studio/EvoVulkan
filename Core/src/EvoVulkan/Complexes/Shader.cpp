//
// Created by Nikita on 08.05.2021.
//

#include <EvoVulkan/Complexes/Shader.h>

#include <EvoVulkan/Tools/StringUtils.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Tools/FileSystem.h>

EvoVulkan::Complexes::Shader::Shader(const EvoVulkan::Types::Device* device, Types::RenderPass renderPass, const VkPipelineCache& cache) {
    m_device     = device;
    m_renderPass = renderPass;
    m_cache      = cache;
}

bool EvoVulkan::Complexes::Shader::Load(
        const std::string& cache,
        const std::vector<SourceShader> &modules,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
        const std::vector<VkDeviceSize>& uniformSizes)
{
    if (modules.empty()) {
        VK_ERROR("Shader::Load() : empty modules list!");
        return false;
    }

    auto modulePatches = std::string();
    for (const auto& module : modules)
        modulePatches += "\n\t" + module.m_path;

    VK_LOG("Shader::Load() : load new shader! Modules:" + modulePatches);

    m_uniformSizes = uniformSizes;
    m_layoutBindings = descriptorLayoutBindings;

    for (const auto& [name, path, stage] : modules) {
        const auto&& outFolder = std::string(cache).append("/") + name;

        std::string file;

        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                file = outFolder + "/vertex.spv";
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                file = outFolder + "/fragment.spv";
                break;
            default:
                VK_ERROR("Shader::Load() : unknown shader stage!");
                return false;
        }

        if (Tools::FileExists(file)) {
            Tools::RemoveFile(file);
        }

        Tools::CreatePath(outFolder);

        system(std::string((g_glslc + " -c ").append(path).append(" -o " + file)).c_str());

        auto shaderModule = Tools::LoadShaderModule(file.c_str(), *m_device);
        if (shaderModule == VK_NULL_HANDLE) {
            VK_ERROR("Shader::Load() : failed to load shader module! \n\tPath: " + file);
            return false;
        }
        else {
            m_shaderModules.push_back(shaderModule);
            m_shaderStages.push_back(Tools::Initializers::PipelineShaderStageCreateInfo(shaderModule, stage));
        }
    }

    return true;
}

std::string EvoVulkan::Complexes::Shader::g_glslc = "None";

bool EvoVulkan::Complexes::Shader::SetVertexDescriptions(
        const std::vector<VkVertexInputBindingDescription> &binding,
        const std::vector<VkVertexInputAttributeDescription> &attribute)
{
    for (uint32_t i = 0; i < binding.size(); i++)
        if (binding[i].binding != i || binding[i].stride <= 0) {
            VK_ERROR("Shader::SetVertexDescriptions() : incorrect vertex binding!");
            return false;
        }

    m_vertices.m_bindingDescriptions   = binding;
    m_vertices.m_attributeDescriptions = attribute;

    m_vertices.m_inputState = Tools::Initializers::PipelineVertexInputStateCreateInfo();
    m_vertices.m_inputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_vertices.m_bindingDescriptions.size());
    m_vertices.m_inputState.pVertexBindingDescriptions      = m_vertices.m_bindingDescriptions.data();
    m_vertices.m_inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertices.m_attributeDescriptions.size());
    m_vertices.m_inputState.pVertexAttributeDescriptions    = m_vertices.m_attributeDescriptions.data();

    m_hasVertices = true;

    return true;
}

bool EvoVulkan::Complexes::Shader::ReCreatePipeLine(Types::RenderPass renderPass) {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(*m_device, m_pipeline, VK_NULL_HANDLE);
        m_pipeline = VK_NULL_HANDLE;
    }

    m_renderPass = renderPass;

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {};

    for (uint32_t i = 0; i < m_renderPass.m_countColorAttach; ++i) {
        auto writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        auto attch = Tools::Initializers::PipelineColorBlendAttachmentState(writeMask, m_blendEnable);

        attch.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attch.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        attch.colorBlendOp        = VK_BLEND_OP_ADD;

        attch.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        attch.alphaBlendOp        = VK_BLEND_OP_ADD;

        blendAttachmentStates.push_back(attch);
    }

    std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    auto&& dynamicState       = Tools::Initializers::PipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
    auto&& colorBlendState    = Tools::Initializers::PipelineColorBlendStateCreateInfo(m_renderPass.m_countColorAttach, blendAttachmentStates.data());
    auto&& pipelineCreateInfo = Tools::Initializers::PipelineCreateInfo(m_pipelineLayout, m_renderPass.m_self, 0);

    pipelineCreateInfo.pVertexInputState   = &m_vertices.m_inputState;
    pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &m_rasterizationState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pMultisampleState   = &m_multisampleState;
    pipelineCreateInfo.pViewportState      = &m_viewportState;
    pipelineCreateInfo.pDepthStencilState  = &m_depthStencilState;
    pipelineCreateInfo.pDynamicState       = &dynamicState;
    pipelineCreateInfo.stageCount          = static_cast<uint32_t>(m_shaderStages.size());
    pipelineCreateInfo.pStages             = m_shaderStages.data();

    if (vkCreateGraphicsPipelines(*m_device, m_cache, 1, &pipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        VK_ERROR("Shader::ReCreatePipeLine() : failed to create vulkan graphics pipeline!");
        return false;
    }

    return true;
}

bool EvoVulkan::Complexes::Shader::Compile(
        VkPolygonMode polygonMode,
        VkCullModeFlags cullMode,
        VkCompareOp depthCompare,
        VkBool32 blendEnable,
        VkBool32 depthWrite,
        VkBool32 depthTest,
        VkPrimitiveTopology topology)
{
    if (!BuildLayouts()) {
        VK_ERROR("Shader::Compile() : failed to build layouts!");
        return false;
    }

    m_blendEnable = blendEnable;

    m_inputAssemblyState = Tools::Initializers::PipelineInputAssemblyStateCreateInfo(topology, 0, VK_FALSE);
    m_rasterizationState = Tools::Initializers::PipelineRasterizationStateCreateInfo(polygonMode, cullMode, VK_FRONT_FACE_CLOCKWISE, 0);

    m_depthStencilState = Tools::Initializers::PipelineDepthStencilStateCreateInfo(depthTest, depthWrite, depthCompare);
    m_viewportState = Tools::Initializers::PipelineViewportStateCreateInfo(1, 1, 0);
    m_multisampleState = Tools::Initializers::PipelineMultisampleStateCreateInfo(m_device->GetMSAASamples(), 0);

    if (!m_hasVertices)
        m_vertices.m_inputState = Tools::Initializers::PipelineVertexInputStateCreateInfo();

    if (!ReCreatePipeLine(m_renderPass)) {
        VK_ERROR("Shader::Compile() : failed to create pipe line!");
        return false;
    }

    return true;
}

bool EvoVulkan::Complexes::Shader::BuildLayouts() {
    m_descriptorSetLayout = Tools::CreateDescriptorLayout(*m_device, m_layoutBindings);
    if (m_descriptorSetLayout == VK_NULL_HANDLE) {
        VK_ERROR("Shader::BuildLayouts() : failed to create descriptor layout!");
        return false;
    }

    m_pipelineLayout = Tools::CreatePipelineLayout(*m_device, m_descriptorSetLayout);
    if (m_pipelineLayout == VK_NULL_HANDLE) {
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

    for (auto&& module : m_shaderModules) {
        vkDestroyShaderModule(*m_device, module, nullptr);
    }
    m_shaderModules.clear();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(*m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    m_cache = VK_NULL_HANDLE;
}

void EvoVulkan::Complexes::Shader::Free() {
    delete this;
}