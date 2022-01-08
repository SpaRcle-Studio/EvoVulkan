//
// Created by Nikita on 06.05.2021.
//

#include <EvoVulkan/DescriptorManager.h>

#include <EvoVulkan/Tools/VulkanDebug.h>

EvoVulkan::Core::DescriptorSet EvoVulkan::Core::DescriptorManager::AllocateDescriptorSets(
        VkDescriptorSetLayout layout,
        const std::set<VkDescriptorType>& requestTypes)
{
    int64_t          _free  = -2;
    DescriptorPool*  _pool  = nullptr;
    DescriptorSet    _set   = { VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr, UINT32_MAX };

    for (auto pool : m_pools) {
        if (pool->m_layout == layout && pool->m_maxSets != pool->m_used && pool->Equal(requestTypes)) {
            pool->m_used++;

            _free = pool->FindFree();
            _pool = pool;

            if (_free < 0) {
                VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
                return _set;
            }
        } else
            continue;
    }

    if (_free < 0) {
        VK_LOG("DescriptorManager::AllocateDescriptor() : create new descriptor pool... Total: " + std::to_string(m_pools.size()));

        _pool = DescriptorPool::Create(m_countDescriptorsAllocate, layout, *m_device, requestTypes);
        if (!_pool) {
            VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to create descriptor pool!");
            return _set;
        }

        m_pools.insert(_pool);

        _free = _pool->FindFree();
        _pool->m_used++;

        if (_free < 0) {
            VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
            return _set;
        }
    }

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    auto descriptorSetAllocInfo = Tools::Initializers::DescriptorSetAllocateInfo(_pool->m_pool, &layout, 1);
    auto result = vkAllocateDescriptorSets(*m_device, &descriptorSetAllocInfo, &descriptorSet);
    if (result != VK_SUCCESS) {
        VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to allocate vulkan descriptor sets!");
        return Core::DescriptorSet();
    }

    _pool->m_descriptorSets[_free] = descriptorSet;

    return _set = { descriptorSet, layout, _pool, static_cast<uint32_t>(_free) };
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

    if (!descriptorSet.m_pool) {
        VK_ERROR("DescriptorManager::FreeDescriptorSet() : descriptor pool is nullptr!");
        return false;
    }

    DescriptorPool* pool = descriptorSet.m_pool;

    if (pool->m_layout != descriptorSet.m_layout) {
        VK_ERROR("DescriptorManager::FreeDescriptorSet() : count used pool is zero! Something went wrong!");
        return false;
    }

    VkDescriptorSet& set = pool->m_descriptorSets[descriptorSet.m_id];
    if (!set || set != descriptorSet.m_self) {
        VK_ERROR("DescriptorManager::FreeDescriptorSet() : descriptor set isn't valid! Something went wrong!");
        return false;
    }

    if (vkFreeDescriptorSets(*m_device, pool->m_pool, 1, &set) != VK_SUCCESS)
        VK_ERROR("DescriptorManager::FreeDescriptorSet() : failed to free vulkan descriptor set!");

    --pool->m_used;
    set = VK_NULL_HANDLE; /// WARN: действительно ли затирается значение по ссылке?

    if (pool->m_used == 0) {
        delete pool;
        m_pools.erase(pool);
        VK_LOG("DescriptorManager::FreeDescriptorSet() : free descriptor pool...");
    }

    return true;
}

EvoVulkan::Core::DescriptorPool::~DescriptorPool()  {
    if (m_descriptorSets) {
        free(m_descriptorSets);
        m_descriptorSets = nullptr;
    }

    if (m_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}
