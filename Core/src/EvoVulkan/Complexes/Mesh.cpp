//
// Created by Nikita on 08.05.2021.
//

#include <EvoVulkan/Complexes/Mesh.h>
#include <EvoVulkan/Types/VulkanBuffer.h>

void EvoVulkan::Complexes::Mesh::Draw(const VkCommandBuffer& cmd) {
    VkDeviceSize offsets[1] = {0};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_attachShader->GetPipelineLayout(), 0, 1, &m_descriptorSet.descriptorSet, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, m_vertices->GetCRef(), offsets);
    vkCmdBindIndexBuffer(cmd, *m_indices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, m_countIndices, 1, 0, 0, 0);
}

bool EvoVulkan::Complexes::Mesh::Bake(Shader const* shader) {
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
