//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_SHADER_H
#define EVOVULKAN_SHADER_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/VulkanBuffer.h>

namespace EvoVulkan::Complexes {
    inline bool file_exists (const std::string& name) {
        struct stat buffer{};
        return (stat (name.c_str(), &buffer) == 0);
    }

    class Shader {
    public:
        Shader(const Types::Device* device, const VkRenderPass& renderPass, const VkPipelineCache& cache);
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
    public:
        bool Load(
                const std::string& source, const std::string& cache,
                const std::vector<std::pair<const char*, VkShaderStageFlagBits>>& modules,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptorLayoutBindings,
                const std::vector<VkDeviceSize>& uniformSizes);
        bool SetVertexDescriptions(
                const std::vector<VkVertexInputBindingDescription>& binding,
                const std::vector<VkVertexInputAttributeDescription>& attribute);
        bool Compile(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkCompareOp depthCompare, VkBool32 blendEnable, VkBool32 depthEnable);

        void Destroy();
        void Free();
    private:
        bool BuildLayouts();
    private:
        struct {
            VkPipelineVertexInputStateCreateInfo           m_inputState;
            std::vector<VkVertexInputBindingDescription>   m_bindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
        } m_vertices;

        const Types::Device*                         m_device              = nullptr;
        VkRenderPass                                 m_renderPass          = VK_NULL_HANDLE;

        VkDescriptorSetLayout                        m_descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding>    m_layoutBindings      = {};
        std::vector<VkDeviceSize>                    m_uniformSizes        = {};

        /** \brief cache is reference. */
        VkPipelineCache                              m_cache               = VK_NULL_HANDLE;
        VkPipeline                                   m_pipeline            = VK_NULL_HANDLE;
        VkPipelineLayout                             m_pipelineLayout      = VK_NULL_HANDLE;

        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages        = {};
        std::vector<VkShaderModule>                  m_shaderModules       = {};
    };
}

#endif //EVOVULKAN_SHADER_H