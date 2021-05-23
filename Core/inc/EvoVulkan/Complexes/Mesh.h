//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_MESH_H
#define EVOVULKAN_MESH_H

#include <EvoVulkan/Complexes/Shader.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/VulkanBuffer.h>

#include <EvoVulkan/DescriptorManager.h>

#include <variant>

namespace EvoVulkan::Complexes {
    static bool IsDescriptorTypeSampler(const VkDescriptorType& type) {
        return type >= VK_DESCRIPTOR_TYPE_SAMPLER && type <= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }

    static VkImageUsageFlags DescriptorTypeToImagesUsage(const VkDescriptorType& type) {
        switch (type) {
            // TODO: see
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                return VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                return VK_IMAGE_USAGE_SAMPLED_BIT;

            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return VK_IMAGE_USAGE_STORAGE_BIT;

            default:
                return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    static VkBufferUsageFlagBits DescriptorTypeToBufferUsage(const VkDescriptorType& type) {
        switch (type) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

            ///TODO: case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            ///TODO: case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:

            default:
                return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    class Mesh {
    private:
        std::vector<std::variant<VkDescriptorImageInfo, Types::Buffer*>> m_uniforms;
    private:
        /** \note reference */
        Types::Buffer const*     m_vertices          = nullptr;
        /** \note reference */
        Types::Buffer const*     m_indices           = nullptr;
        uint32_t                 m_countIndices      = 0;

        //VkDescriptorSet          m_descriptorSet     = VK_NULL_HANDLE;
        Core::DescriptorSet      m_descriptorSet     = { };

        //Types::Buffer**          m_uniforms          = nullptr;
        //uint32_t                 m_countUniforms     = 0;
        //VkDescriptorImageInfo*   m_samplers          = nullptr;
        //uint32_t                 m_countSamplers     = 0;

        const Types::Device*     m_device            = nullptr;

        Core::DescriptorManager* m_descriptorManager = nullptr;

        Shader const*            m_attachShader      = nullptr;
    public:
        void Draw(const VkCommandBuffer& cmd);

        Mesh(const Types::Device* device, Types::Buffer const *vertices, Types::Buffer const *indices, const uint32_t& countIndices, Core::DescriptorManager* manager);

        bool Bake(Shader const* shader);
    };
}

#endif //EVOVULKAN_MESH_H
