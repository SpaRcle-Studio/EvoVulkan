//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_SHADER_H
#define EVOVULKAN_SHADER_H

#include <sys/stat.h>
//#include <unistd.h>
#include <string>
#include <fstream>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/RenderPass.h>
#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/VulkanBuffer.h>

namespace EvoVulkan::Complexes {
    class Shader {
    public:
        Shader(const Types::Device* device, Types::RenderPass renderPass, const VkPipelineCache& cache);
    public:
        /**
         * @note Use for building descriptors
         */
        [[nodiscard]] inline VkDescriptorSetLayout GetDescriptorSetLayout() const noexcept {
            return m_descriptorSetLayout;
        }

        [[nodiscard]] inline std::set<VkDescriptorType> GetDescriptorTypes() const {
            auto types = std::set<VkDescriptorType>();
            for (VkDescriptorSetLayoutBinding binding : m_layoutBindings)
                types.insert(binding.descriptorType);
            return types;
        }

        [[nodiscard]] inline std::vector<VkDeviceSize> GetUniformSizes() const {
            return m_uniformSizes;
        }

        [[nodiscard]] inline VkPipeline GetPipeline() const noexcept {
            return m_pipeline;
        }

        [[nodiscard]] inline VkPipelineLayout GetPipelineLayout() const noexcept {
            return m_pipelineLayout;
        }

        inline void Bind(const VkCommandBuffer& cmd) const {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_pipeline);
        }
    public:
        bool Load(
                const std::string& source, const std::string& cache,
                const std::vector<std::pair<std::string, VkShaderStageFlagBits>>& modules,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
                const std::vector<VkDeviceSize>& uniformSizes);
        bool SetVertexDescriptions(
                const std::vector<VkVertexInputBindingDescription>& binding,
                const std::vector<VkVertexInputAttributeDescription>& attribute);
        bool Compile(
                VkPolygonMode polygonMode,
                VkCullModeFlags cullMode,
                VkCompareOp depthCompare,
                VkBool32 blendEnable,
                VkBool32 depthWrite,
                VkBool32 depthTest);

        void Destroy();
        void Free();

        bool ReCreatePipeLine(Types::RenderPass renderPass);
    private:
        bool BuildLayouts();
    public:
        static void SetGlslCompiler(const std::string& glslc) {
            g_glslc = glslc;
        }
    public:
        operator VkPipeline() const {
            return m_pipeline;
        }
    private:
        inline static std::string g_glslc = "None";

        struct {
            VkPipelineVertexInputStateCreateInfo           m_inputState;
            std::vector<VkVertexInputBindingDescription>   m_bindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
        } m_vertices;

        const Types::Device*                         m_device              = nullptr;
        //VkRenderPass                                 m_renderPass          = VK_NULL_HANDLE;
        Types::RenderPass                            m_renderPass          = { };

        VkDescriptorSetLayout                        m_descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding>    m_layoutBindings      = {};

        bool                                         m_hasVertices         = false;

        /** \brief cache is reference. */
        VkPipelineCache                              m_cache               = VK_NULL_HANDLE;
        VkPipeline                                   m_pipeline            = VK_NULL_HANDLE;
        VkPipelineLayout                             m_pipelineLayout      = VK_NULL_HANDLE;

        VkBool32                                     m_blendEnable         = VK_FALSE;

        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages        = {};
        std::vector<VkShaderModule>                  m_shaderModules       = {};

        std::vector<VkDeviceSize>                    m_uniformSizes        = {};

        VkPipelineInputAssemblyStateCreateInfo       m_inputAssemblyState  = {};
        VkPipelineRasterizationStateCreateInfo       m_rasterizationState  = {};
        VkPipelineDepthStencilStateCreateInfo        m_depthStencilState   = {};
        VkPipelineViewportStateCreateInfo            m_viewportState       = {};
        VkPipelineMultisampleStateCreateInfo         m_multisampleState    = {};
    };
}

#endif //EVOVULKAN_SHADER_H
