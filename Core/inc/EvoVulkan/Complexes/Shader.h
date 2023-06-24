//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_SHADER_H
#define EVOVULKAN_SHADER_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/RenderPass.h>
#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/VulkanBuffer.h>
#include <EvoVulkan/Tools/Singleton.h>

namespace EvoVulkan::Complexes {
    struct DLL_EVK_EXPORT SourceShader {
        std::string m_path;
        VkShaderStageFlagBits m_type;

        SourceShader(const std::string& path, VkShaderStageFlagBits type) {
            m_path = path;
            m_type = type;
        }
    };

    class DLL_EVK_EXPORT GLSLCompiler : public Tools::Singleton<GLSLCompiler> {
        friend class Tools::Singleton<GLSLCompiler>;
    protected:
        ~GLSLCompiler() override = default;

    public:
        void Init(std::string path) {
            m_compiler = std::move(path);
        }

        EVK_NODISCARD std::string GetPath() const {
            return m_compiler;
        }

    private:
        std::string m_compiler;

    };

    class DLL_EVK_EXPORT Shader : public Tools::NonCopyable {
        using Super = Tools::NonCopyable;
    public:
        Shader(const Types::Device* device, Types::RenderPass renderPass, const VkPipelineCache& cache);
        ~Shader() override;

        operator VkPipeline() const { return m_pipeline; }

    public:
        bool Load(
            const std::string& cache,
            const std::vector<SourceShader>& modules,
            const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
            const std::vector<VkPushConstantRange>& pushConstants
        );

        bool SetVertexDescriptions(
                const std::vector<VkVertexInputBindingDescription>& binding,
                const std::vector<VkVertexInputAttributeDescription>& attribute);

        bool Compile(
                VkPolygonMode polygonMode,
                VkCullModeFlags cullMode,
                VkCompareOp depthCompare,
                VkBool32 blendEnable,
                VkBool32 depthWrite,
                VkBool32 depthTest,
                VkPrimitiveTopology topology,
                VkSampleCountFlagBits rasterizationSamples);

    public:
        /**
         * @note Use for building descriptors
         */
        EVK_NODISCARD EVK_INLINE VkDescriptorSetLayout GetDescriptorSetLayout() const noexcept { return m_descriptorSetLayout; }
        EVK_NODISCARD EVK_INLINE VkPipeline GetPipeline() const noexcept { return m_pipeline; }
        EVK_NODISCARD EVK_INLINE VkPipelineLayout GetPipelineLayout() const noexcept { return m_pipelineLayout; }
        EVK_NODISCARD EVK_INLINE const std::vector<VkPushConstantRange>& GetPushConstants() const noexcept { return m_pushConstants; }

        void Bind(const VkCommandBuffer& cmd) const;

        bool ReCreatePipeLine(Types::RenderPass renderPass);

    private:
        bool BuildLayouts();

    private:
        struct {
            VkPipelineVertexInputStateCreateInfo           m_inputState;
            std::vector<VkVertexInputBindingDescription>   m_bindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
        } m_vertices;

        const Types::Device*                          m_device              = nullptr;
        Types::RenderPass                             m_renderPass          = { };

        VkDescriptorSetLayout                         m_descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding>     m_layoutBindings      = { };

        bool                                          m_hasVertices         = false;

        /** \brief cache is reference. */
        VkPipelineCache                               m_cache               = VK_NULL_HANDLE;
        VkPipeline                                    m_pipeline            = VK_NULL_HANDLE;
        VkPipelineLayout                              m_pipelineLayout      = VK_NULL_HANDLE;

        VkBool32                                      m_blendEnable         = VK_FALSE;

        std::vector<VkPipelineShaderStageCreateInfo>  m_shaderStages        = { };
        std::vector<VkShaderModule>                   m_shaderModules       = { };

        std::vector<VkPushConstantRange>              m_pushConstants       = { };

        VkPipelineInputAssemblyStateCreateInfo        m_inputAssemblyState  = { };
        VkPipelineRasterizationStateCreateInfo        m_rasterizationState  = { };
        VkPipelineDepthStencilStateCreateInfo         m_depthStencilState   = { };
        VkPipelineViewportStateCreateInfo             m_viewportState       = { };
        VkPipelineMultisampleStateCreateInfo          m_multisampleState    = { };
        VkPipelineRasterizationLineStateCreateInfoEXT m_lineState           = { };

    };
}

#endif //EVOVULKAN_SHADER_H
