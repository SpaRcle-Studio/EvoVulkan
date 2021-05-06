//
// Created by Nikita on 06.05.2021.
//

#include "EvoVulkan/DescriptorManager.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

VkDescriptorSet EvoVulkan::Core::DescriptorManager::AllocateDescriptor(VkDescriptorSetLayout layout) {
    VK_LOG("DescriptorManager::AllocateDescriptor() : allocate new descriptor...");

    VkDescriptorSet* _free  = nullptr;
    DescriptorPool*  _pool  = nullptr;
    uint32_t         _index = 0;

    for (auto pool : m_pools) {
        if (pool->m_layout == layout && pool->m_maxSets != pool->m_used) {
            _index = pool->m_used;

            pool->m_used++;

            _free = pool->FindFree();
            _pool = pool;

            if (_free == nullptr) {
                VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
                return VK_NULL_HANDLE;
            }
        } else
            continue;
    }

    if (!_free) {
        VK_LOG("DescriptorManager::AllocateDescriptor() : create new descriptor pool...");

        _pool = DescriptorPool::Create(m_countDescriptorsAllocate, layout, *m_device);
        if (!_pool) {
            VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to create descriptor pool!");
            return nullptr;
        }

        m_pools.emplace_back(_pool);

        _free = _pool->FindFree();

        if (_free == nullptr) {
            VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
            return VK_NULL_HANDLE;
        }
    }

    auto descriptorSetAllocInfo = Tools::Initializers::DescriptorSetAllocateInfo(_pool->m_pool, &layout, 1);
    auto result = vkAllocateDescriptorSets(*m_device, &descriptorSetAllocInfo, _free);
    if (result != VK_SUCCESS) {
        VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to allocate vulkan descriptor set!");
        return nullptr;
    }

    _pool->m_descriptorSets[_index] = *_free;

    return *_free;
}

void EvoVulkan::Core::DescriptorManager::Reset() {
    for (auto pool : m_pools)
        delete pool;

    m_pools.clear();
}
