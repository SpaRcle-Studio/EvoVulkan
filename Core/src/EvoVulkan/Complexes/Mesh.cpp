//
// Created by Nikita on 08.05.2021.
//

#include <variant>

#include <EvoVulkan/Complexes/Mesh.h>

void EvoVulkan::Complexes::Mesh::Draw(const VkCommandBuffer& cmd) {
    VkDeviceSize offsets[1] = {0};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            this->m_attachShader->GetPipelineLayout(), 0, 1, &m_descriptorSet.m_self, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertices->m_buffer, offsets);
    vkCmdBindIndexBuffer(cmd, m_indices->m_buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, m_countIndices, 1, 0, 0, 0);
}

bool EvoVulkan::Complexes::Mesh::Bake(Shader const* shader) {
   /* this->m_attachShader = shader;

    m_descriptorSet = this->m_descriptorManager->AllocateDescriptorSets(
            shader->GetDescriptorSetLayout(),
            m_attachShader->GetDescriptorTypes()
    );
    if (m_descriptorSet.m_self == VK_NULL_HANDLE) {
        VK_ERROR("Mesh::Bake() : failed to allocate descriptor sets!");
        return false;
    }

    int buffID = 0;

    for (VkDescriptorType type : m_attachShader->GetDescriptorTypes()) {
        if (IsDescriptorTypeSampler(type)) {
            auto image  = DescriptorTypeToImagesUsage(type);
            if (image == VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM) {
                VK_ERROR("Mesh::Bake() : incorrect image descriptor!");
                return false;
            }
            VkDescriptorImageInfo textureDescriptor;

            this->m_uniforms.emplace_back(textureDescriptor);

            VK_WARN("Mesh::Bake() : texture using in not complete! TODO!");
        }
        else {
            auto buffer =  DescriptorTypeToBufferUsage(type);
            if (buffer == VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM) {
                VK_ERROR("Mesh::Bake() : incorrect buffer descriptor!");
                return false;
            }

            this->m_uniforms.emplace_back(EvoVulkan::Types::Buffer::Create(
                    m_device,
                    buffer,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                    m_attachShader->GetUniformSizes()[buffID]));

            buffID += 1;
        }
    }

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {};
    for (uint32_t i = 0; i < m_uniforms.size(); i++) {
        auto uniform = m_uniforms[i];

        VkDescriptorType type = *std::next(m_attachShader->GetDescriptorTypes().begin(), i);

        if (uniform.index() == 0) { // image
            VkDescriptorImageInfo* image = &uniform._Storage()._Head;

        } else { // buffer
            Types::Buffer* buffer = uniform._Storage()._Tail._Get();

            writeDescriptorSets.push_back(
                    Tools::Initializers::WriteDescriptorSet(
                            m_descriptorSet.m_self,
                            type,
                            0, &buffer->m_descriptor));
        }
    }
    vkUpdateDescriptorSets(*m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
*/

    return true;
}

EvoVulkan::Complexes::Mesh::Mesh(
        const Types::Device* device,
        Types::Buffer const *vertices,
        Types::Buffer const *indices,
        const uint32_t& countIndices,
        Core::DescriptorManager* manager)
{
    this->m_descriptorManager = manager;

    this->m_device            = device;

    this->m_vertices          = vertices;
    this->m_indices           = indices;
    this->m_countIndices      = countIndices;
}
