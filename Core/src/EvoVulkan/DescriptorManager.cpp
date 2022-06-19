//
// Created by Nikita on 06.05.2021.
//

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Tools/VulkanDebug.h>

namespace EvoVulkan::Core {
    const std::vector<std::pair<VkDescriptorType, float>> g_poolSizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f  },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.f  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.f  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.f  },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f  },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f  },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f }
    };

    DescriptorSet DescriptorManager::AllocateDescriptorSets(VkDescriptorSetLayout layout, const std::set<VkDescriptorType>& requestTypes) {
        int64_t          _free  = EVK_ID_INVALID;
        DescriptorPool*  _pool  = nullptr;
        DescriptorSet    _set   = { VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr, UINT32_MAX };

        for (auto pool : m_pools) {
            if (pool->m_layout == layout && pool->m_maxSets != pool->m_used && pool->Equal(requestTypes)) {
                pool->m_used++;

                _free = pool->FindFree();
                _pool = pool;

                if (_free == EVK_ID_INVALID) {
                    VK_ERROR("DescriptorManager::AllocateDescriptor() : something went wrong!");
                    return _set;
                }
            }
            else
                continue;
        }

        if (_free == EVK_ID_INVALID) {
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
            VK_ERROR("DescriptorManager::AllocateDescriptor() : failed to allocate vulkan descriptor sets!"
                     "\n\tReason: " + Tools::Convert::result_to_string(result) +
                     "\n\tDescription: " + Tools::Convert::result_to_description(result));
            return Core::DescriptorSet();
        }

        _pool->m_descriptorSets[_free] = descriptorSet;

        return _set = DescriptorSet { descriptorSet, layout, _pool, static_cast<uint32_t>(_free) };
    }

    void EvoVulkan::Core::DescriptorManager::Reset() {
        for (auto&& pool : m_pools) {
            delete pool;
        }

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

    void DescriptorManager::Free() {
        VK_LOG("DescriptorManager::Free() : free descriptor manager pointer...");

        if (!m_pools.empty()) {
            std::string str;
            uint32_t index = 0;
            for (const auto& pool : m_pools) {
                str += "\n\t[" + std::to_string(index) + "] = " + std::to_string(pool->m_used) + " descriptor sets";
                ++index;
            }

            VK_WARN("DescriptorManager::Free() : not all descriptor pools have been freed!" + str);
        }

        Reset();

        delete this;
    }

    DescriptorManager *DescriptorManager::Create(const EvoVulkan::Types::Device *device) {
        auto&& manager = new DescriptorManager();
        manager->m_device = device;
        return manager;
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

    bool EvoVulkan::Core::DescriptorPool::Contains(const std::set<VkDescriptorType> &types, const VkDescriptorType &type) {
        std::set<VkDescriptorType>::iterator it;
        for (it = types.begin(); it != types.end(); ++it)
            if (type == *it)
                return true;

        return false;
    }

    DescriptorPool* DescriptorPool::Create(EvoVulkan::Types::Device *device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize> &poolSizes) {
        auto&& pool = new DescriptorPool(maxSets);

        pool->m_layout         = VK_NULL_HANDLE;
        pool->m_device         = *device;
        pool->m_requestTypes   = {};
        pool->m_descriptorSets = nullptr;

        auto dSizes = poolSizes;
        auto descriptorPoolCI = Tools::Initializers::DescriptorPoolCreateInfo(dSizes.size(), dSizes.data(), maxSets);

        descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult vkRes = vkCreateDescriptorPool(*device, &descriptorPoolCI, nullptr, &pool->m_pool);
        if (vkRes != VK_SUCCESS) {
            VK_ERROR("DescriptorPool::Create() : failed to create vulkan descriptor pool!");
            return VK_NULL_HANDLE;
        }

        return pool;
    }

    DescriptorPool *DescriptorPool::Create(uint32_t maxSets, VkDescriptorSetLayout layout, VkDevice device, const std::set<VkDescriptorType> &requestTypes) {
        if (requestTypes.empty()) {
            VK_ERROR("DescriptorPool::Create() : request types is empty!");
            return nullptr;
        }

        auto* pool = new DescriptorPool(maxSets);

        pool->m_layout = layout;
        pool->m_device = device;

        pool->m_requestTypes = requestTypes;

        pool->m_descriptorSets = (VkDescriptorSet *) malloc(sizeof(VkDescriptorSet) * maxSets);
        for (uint32_t i = 0; i < maxSets; i++)
            pool->m_descriptorSets[i] = VK_NULL_HANDLE;

        std::vector<VkDescriptorPoolSize> sizes = {};
        sizes.reserve(g_poolSizes.size());
        for (auto sz : g_poolSizes)
            if (Contains(requestTypes, sz.first))
                sizes.push_back({ sz.first, uint32_t(sz.second * maxSets) });

        VkDescriptorPoolCreateInfo descriptorPoolCI =
                Tools::Initializers::DescriptorPoolCreateInfo(sizes.size(), sizes.data(), maxSets);

        descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &pool->m_pool);
        if (vkRes != VK_SUCCESS) {
            VK_ERROR("DescriptorPool::Create() : failed to create vulkan descriptor pool!");
            return VK_NULL_HANDLE;
        }

        return pool;
    }

    bool DescriptorPool::Equal(const std::set<VkDescriptorType> &requestTypes) {
        return requestTypes.size() == m_requestTypes.size() && std::equal(requestTypes.begin(), requestTypes.end(), m_requestTypes.begin());
    }

    int64_t DescriptorPool::FindFree() const {
        for (uint32_t i = 0; i < m_maxSets; ++i) {
            if (m_descriptorSets[i] == VK_NULL_HANDLE) {
                return i;
            }
        }

        return EVK_ID_INVALID;
    }
}