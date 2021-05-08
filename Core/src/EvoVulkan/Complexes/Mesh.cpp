//
// Created by Nikita on 08.05.2021.
//

#include <EvoVulkan/Complexes/Mesh.h>

void EvoVulkan::Complexes::Mesh::Draw() {

}

bool EvoVulkan::Complexes::Mesh::Bake(Shader const* shader) {
    this->m_attachShader = shader;

    std::vector<VkBufferUsageFlagBits> buffers = {};
    std::vector<VkImageUsageFlags>     images  = {};

    for (VkDescriptorType type : m_attachShader->GetDescriptorTypes()) {
        if (IsDescriptorTypeSampler(type))
            images.push_back(DescriptorTypeToImagesUsage(type));
        else {
            VkBufferUsageFlagBits usage = DescriptorTypeToBufferUsage(type);
            if (usage == VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM) {
                VK_ERROR("Mesh::Bake() : incorrect descriptor type!");
                return false;
            } else
                buffers.push_back(usage);
        }
    }

    m_descriptorSet = this->m_descriptorManager->AllocateDescriptorSets(
            shader->GetDescriptorSetLayout(),
            m_attachShader->GetDescriptorTypes()
    );
    if (m_descriptorSet == VK_NULL_HANDLE) {
        VK_ERROR("Mesh::Bake() : failed to allocate descriptor sets!");
        return false;
    }

    this->m_countUniforms = buffers.size();
    if (m_countUniforms) {
        this->m_uniforms = (Types::Buffer **) malloc(sizeof(Types::Buffer) * m_countUniforms);
        for (uint32_t i = 0; i < m_countUniforms; i++) {
            m_uniforms[i] = EvoVulkan::Types::Buffer::Create(
                    m_device,
                    buffers[i],
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                    m_attachShader->GetUniformSizes()[i]);
        }
    }

    this->m_countSamplers = images.size();
    if (m_countSamplers) {
        this->m_samplers = (VkImage)
    }

    return true;
}

EvoVulkan::Complexes::Mesh::Mesh(
        const Types::Device* device,
        Types::Buffer const *vertices,
        Types::Buffer const *indices,
        Core::DescriptorManager* manager)
{
    this->m_descriptorManager = manager;

    this->m_device   = device;

    this->m_vertices = vertices;
    this->m_indices  = indices;
}
