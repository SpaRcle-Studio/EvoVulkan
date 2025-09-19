//
// Created by Nikita on 08.05.2021.
//

#include <EvoVulkan/Complexes/Shader.h>

#include <EvoVulkan/Tools/StringUtils.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Tools/FileSystem.h>

EvoVulkan::Complexes::Shader::Shader(const EvoVulkan::Types::Device* pDevice, Types::RenderPass renderPass, const VkPipelineCache& cache)
    : Super()
    , m_device(pDevice)
    , m_renderPass(renderPass)
    , m_cache(cache)
{ }

/*
 *         if (EvoVulkan::Tools::VkFunctionsHolder::Instance().IsSupportGLSLang()) {
            if (!EvoVulkan::Tools::VkFunctionsHolder::Instance().CompileGLSLtoSPIRV(inputFile)) {
                VK_ERROR("Shader::Load() : failed to compile shader!\n\tPath: " + inputFile);
                return { VK_NULL_HANDLE, {} };
            }
        }
        else {
 */

std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo> CompileShaderModule(
    const std::string& path,
    const std::string& cache,
    VkShaderStageFlagBits stage,
    const EvoVulkan::Types::Device* device
) {
    const std::string inputFile  = std::string(cache + "/").append(path);
    const std::string hashFile   = inputFile + ".spv.hash";
    const std::string outputFile = inputFile + ".spv";

    const uint64_t hash = EvoVulkan::Tools::VkFunctionsHolder::Instance().GetFileHash(inputFile);

    if (hash != EvoVulkan::Tools::VkFunctionsHolder::Instance().ReadHash(hashFile) || !EvoVulkan::Tools::VkFunctionsHolder::Instance().IsExists(outputFile)) {
        EvoVulkan::Tools::VkFunctionsHolder::Instance().WriteHash(hashFile, hash);

        if (EVK_IS_EXISTS(outputFile)) {
            EVK_DELETE_FILE(outputFile);
        }

        std::string command;

#if defined(EVK_WIN32) || defined(EVK_LINUX)
    #ifdef EVK_WIN32
        command = std::string("\"\"" + (EvoVulkan::Complexes::GLSLCompiler::Instance().GetPath() + "\" -c \"").append(inputFile).append("\" -o \"" + outputFile + "\"\""));
    #else
        command = std::string("\"" + (EvoVulkan::Complexes::GLSLCompiler::Instance().GetPath() + "\" -c \"").append(inputFile).append("\" -o \"" + outputFile + "\""));
    #endif

        EvoVulkan::Tools::VkFunctionsHolder::Instance().ExecuteCommand(command);
#else
        VK_ERROR("CompileShaderModule() : the platform does not support shader compilation!");
        return { VK_NULL_HANDLE, {} };
#endif
        if (!EvoVulkan::Tools::VkFunctionsHolder::Instance().IsExists(outputFile)) {
            VK_ERROR("CompileShaderModule() : failed to compile shader!\n\tPath: " + inputFile + "\n\tGLSL Command: " + command);
            return { VK_NULL_HANDLE, {} };
        }
    }

    auto shaderModule = LoadShaderModule(outputFile.c_str(), *device);
    if (shaderModule == VK_NULL_HANDLE) {
        VK_ERROR("CompileShaderModule() : failed to load shader module! \n\tPath: " + inputFile);
        return { VK_NULL_HANDLE, {} };
    }

    return {
        shaderModule,
        EvoVulkan::Tools::Initializers::PipelineShaderStageCreateInfo(shaderModule, stage)
    };
}

std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo> GLSLLangCompileShaderModule(
    const std::string& path,
    const std::string& cache,
    VkShaderStageFlagBits stage,
    const EvoVulkan::Types::Device* device
) {
    const std::string inputFile  = std::string(cache + "/").append(path);
    const std::string hashFile   = inputFile + ".spv.hash";
    const std::string outputFile = inputFile + ".spv";

    const uint64_t hash = EvoVulkan::Tools::VkFunctionsHolder::Instance().GetFileHash(inputFile);

    std::vector<uint32_t> data;

    if (hash != EvoVulkan::Tools::VkFunctionsHolder::Instance().ReadHash(hashFile) || !EvoVulkan::Tools::VkFunctionsHolder::Instance().IsExists(outputFile)) {
        EvoVulkan::Tools::VkFunctionsHolder::Instance().WriteHash(hashFile, hash);
        data = EvoVulkan::Tools::VkFunctionsHolder::Instance().CompileGLSLtoSPIRV(inputFile);
        EvoVulkan::Tools::VkFunctionsHolder::Instance().WriteSPIRV(outputFile, data);
    }
    else {
        data = EvoVulkan::Tools::VkFunctionsHolder::Instance().ReadSPIRV(outputFile);
    }

    if (data.empty()) {
        VK_ERROR("GLSLLangCompileShaderModule() : failed to compile shader!\n\tPath: " + inputFile);
        return {VK_NULL_HANDLE, {}};
    }

    auto shaderModule = LoadShaderModule(data, *device);
    if (shaderModule == VK_NULL_HANDLE) {
        VK_ERROR("GLSLLangCompileShaderModule() : failed to load shader module! \n\tPath: " + inputFile);
        return { VK_NULL_HANDLE, {} };
    }

    return {
        shaderModule,
        EvoVulkan::Tools::Initializers::PipelineShaderStageCreateInfo(shaderModule, stage)
    };
}

bool EvoVulkan::Complexes::Shader::Load(
    const std::string& cache,
    const std::vector<SourceShader> &modules,
    const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
    const std::vector<VkPushConstantRange>& pushConstants
) {
    if (modules.empty()) {
        VK_ERROR("Shader::Load() : empty modules list!");
        return false;
    }

    /// just a log
    {
        auto modulePatches = std::string();
        for (auto&& module : modules) {
            modulePatches += "\n\t" + module.m_path;
        }
        VK_LOG("Shader::Load() : load new shader! Modules:" + modulePatches);
    }

    m_layoutBindings = descriptorLayoutBindings;
    m_pushConstants = pushConstants;

#define EVK_USE_FUTURE_LOAD_SHADER

#ifdef EVK_USE_FUTURE_LOAD_SHADER
    std::vector<std::future<std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo>>> futures;

    for (const auto& [path, stage] : modules) {
        futures.push_back(std::async(std::launch::async, [&, shaderPath = path, shaderStage = stage]() -> std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo> {
            if (Tools::VkFunctionsHolder::Instance().IsSupportGLSLang()) {
                return GLSLLangCompileShaderModule(shaderPath, cache, shaderStage, m_device);
            }
            return CompileShaderModule(shaderPath, cache, shaderStage, m_device);
        }));
    }

    // собираем результаты
    for (auto& f : futures) {
        auto [shaderModule, stageInfo] = f.get();
        if (shaderModule == VK_NULL_HANDLE) {
            return false; // при ошибке вываливаемся
        }
        m_shaderModules.push_back(shaderModule);
        m_shaderStages.push_back(stageInfo);
    }
#else
    for (const auto& [path, stage] : modules) {
        std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo> result;

        if (Tools::VkFunctionsHolder::Instance().IsSupportGLSLang()) {
            result = GLSLLangCompileShaderModule(path, cache, stage, m_device);
        }
        else {
            result = CompileShaderModule(path, cache, stage, m_device);
        }

        if (result.first == VK_NULL_HANDLE) {
            VK_ERROR("Shader::Load() : failed to compile shader module!\n\tPath: " + path);
            return false;
        }

        m_shaderModules.push_back(result.first);
        m_shaderStages.push_back(result.second);
    }
#endif

    return true;
}

bool EvoVulkan::Complexes::Shader::SetVertexDescriptions(
    const std::vector<VkVertexInputBindingDescription> &binding,
    const std::vector<VkVertexInputAttributeDescription> &attribute
) {
    for (uint32_t i = 0; i < binding.size(); ++i) {
        if (binding[i].binding != i || binding[i].stride <= 0) {
            VK_ERROR("Shader::SetVertexDescriptions() : incorrect vertex binding!");
            return false;
        }
    }

    m_vertices.m_bindingDescriptions = binding;
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
        auto attachment = Tools::Initializers::PipelineColorBlendAttachmentState(writeMask, m_blendEnable);

        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        attachment.colorBlendOp        = VK_BLEND_OP_ADD;
        attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        blendAttachmentStates.push_back(attachment);
    }

    std::vector<VkDynamicState> dynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    auto&& dynamicState    = Tools::Initializers::PipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
    auto&& colorBlendState = Tools::Initializers::PipelineColorBlendStateCreateInfo(m_renderPass.m_countColorAttach, blendAttachmentStates.data());

    auto&& pipelineCreateInfo = Tools::Initializers::PipelineCreateInfo(
        m_pipelineLayout,
        m_renderPass.m_self,
        0
    );

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
    VkPrimitiveTopology topology,
    VkSampleCountFlagBits rasterizationSamples
) {
    if (!BuildLayouts()) {
        VK_ERROR("Shader::Compile() : failed to build layouts!");
        return false;
    }

    m_blendEnable = blendEnable;

    m_inputAssemblyState = Tools::Initializers::PipelineInputAssemblyStateCreateInfo(topology, 0, VK_FALSE);
    m_rasterizationState = Tools::Initializers::PipelineRasterizationStateCreateInfo(polygonMode, cullMode, VK_FRONT_FACE_CLOCKWISE, 0);

    m_lineState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
    m_lineState.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT;
    m_lineState.stippledLineEnable = true;

    /// m_rasterizationState.pNext = &m_lineState;

    m_depthStencilState = Tools::Initializers::PipelineDepthStencilStateCreateInfo(depthTest, depthWrite, depthCompare);
    m_viewportState = Tools::Initializers::PipelineViewportStateCreateInfo(1, 1, 0);
    m_multisampleState = Tools::Initializers::PipelineMultisampleStateCreateInfo(rasterizationSamples, 0);

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

    m_pipelineLayout = Tools::CreatePipelineLayout(*m_device, 1, m_descriptorSetLayout, m_pushConstants);
    if (m_pipelineLayout == VK_NULL_HANDLE) {
        VK_ERROR("Shader::BuildLayouts() : failed to create pipeline layout!");
        return false;
    }

    return true;
}

EvoVulkan::Complexes::Shader::~Shader() {
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

void EvoVulkan::Complexes::Shader::Bind(VkCommandBuffer const &cmd) const {
    if (!m_pipeline || m_shaderStages.empty()) {
        return;
    }

    if (m_shaderStages.front().stage == VK_SHADER_STAGE_COMPUTE_BIT) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    }
    else {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }
}

bool EvoVulkan::Complexes::Shader::CompileCompute() {
    if (m_shaderStages.size() != 1 || m_shaderStages.front().stage != VK_SHADER_STAGE_COMPUTE_BIT || !m_shaderStages.front().module) {
        VK_ERROR("Shader::CompileCompute() : shader stages must contain only one compute shader stage!");
        return false;
    }

    if (!BuildLayouts()) {
        VK_ERROR("Shader::CompileCompute() : failed to build layouts!");
        return false;
    }

    VkComputePipelineCreateInfo computePipelineCreateInfo = Tools::Initializers::ComputePipelineCreateInfo(m_pipelineLayout, 0);
    computePipelineCreateInfo.stage = m_shaderStages.front();

    if (vkCreateComputePipelines(*m_device, m_cache, 1, &computePipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        VK_ERROR("Shader::CompileCompute() : failed to create vulkan graphics pipeline!");
        return false;
    }

    return true;
}
