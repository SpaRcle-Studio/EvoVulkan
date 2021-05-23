//
// Created by Nikita on 06.05.2021.
//

#include "EvoVulkan/DescriptorManager.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

EvoVulkan::Core::DescriptorSet EvoVulkan::Core::DescriptorManager::AllocateDescriptorSets(
        VkDescriptorSetLayout layout,
        const std::set<VkDescriptorType>& requestTypes)
{
    VK_LOG("DescriptorManager::AllocateDescriptor() : allocate new descriptor...");

    //VkDescriptorSet* _free  = NULL;
    int64_t          _free  = -2;
    DescriptorPool*  _pool  = NULL;
    //uint32_t         _index = 0;

    for (auto pool : m_pools) {
        if (pool->m_layout == layout && pool->m_maxSets != pool->m_used && pool->Equal(requestTypes)) {
            //_index = pool->m_used;

            pool->m_used++;

            _free = pool->FindFree();
            _pool = pool;

            if (_free < 0) {
                VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
                return { VK_NULL_HANDLE, VK_NULL_HANDLE };
            }
        } else
            continue;
    }

    if (_free < 0) {
        VK_LOG("DescriptorManager::AllocateDescriptor() : create new descriptor pool...");

        _pool = DescriptorPool::Create(m_countDescriptorsAllocate, layout, *m_device, requestTypes);
        if (!_pool) {
            VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to create descriptor pool!");
            return { VK_NULL_HANDLE, VK_NULL_HANDLE };
        }

        m_pools.emplace_back(_pool);

        _free = _pool->FindFree();
        _pool->m_used++;

        if (_free < 0) {
            VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
            return  { VK_NULL_HANDLE, VK_NULL_HANDLE };
        }
    }

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    auto descriptorSetAllocInfo = Tools::Initializers::DescriptorSetAllocateInfo(_pool->m_pool, &layout, 1);
    auto result = vkAllocateDescriptorSets(*m_device, &descriptorSetAllocInfo, &descriptorSet);
    if (result != VK_SUCCESS) {
        VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to allocate vulkan descriptor sets!");
        return  { VK_NULL_HANDLE, VK_NULL_HANDLE };
    }

    _pool->m_descriptorSets[_free] = descriptorSet;

    return { descriptorSet, layout };
}

void EvoVulkan::Core::DescriptorManager::Reset() {
    for (auto pool : m_pools)
        delete pool;

    m_pools.clear();
}

bool EvoVulkan::Core::DescriptorManager::FreeDescriptorSet(EvoVulkan::Core::DescriptorSet descriptorSet) {
    if (descriptorSet.m_self == VK_NULL_HANDLE || descriptorSet.m_layout == VK_NULL_HANDLE) {
        VK_ERROR("DescriptorManager::FreeDescriptorSet() : descriptor is nullptr!");
        return false;
    }

    for (DescriptorPool* pool : m_pools)
        if (pool->m_layout == descriptorSet.m_layout) {
            if (pool->m_used == 0) {
                VK_ERROR("DescriptorManager::FreeDescriptorSet() : count used pool is zero! Something went wrong!");
                return false;
            }

            for (size_t i = 0; i < pool->m_maxSets; i++)
                if (pool->m_descriptorSets[i] == descriptorSet.m_self) {
                    vkFreeDescriptorSets(*m_device, pool->m_pool, 1, &pool->m_descriptorSets[i]);
                    pool->m_descriptorSets[i] = VK_NULL_HANDLE;

                    pool->m_used--;

                    VK_GRAPH("DescriptorManager::FreeDescriptorSet() : descriptor was been successfully freed!");
                    return true;
                }
        }

    VK_ERROR("DescriptorManager::FreeDescriptorSet() : failed to free descriptor set! (Not found)");

    return false;
}
