//
// Created by Monika on 23.06.2022.
//

#include <EvoVulkan/Types/DescriptorPool.h>
#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Types {
    DescriptorPool::~DescriptorPool()  {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device, m_pool, nullptr);
            m_pool = VK_NULL_HANDLE;
        }
    }

    bool DescriptorPool::Contains(const std::set<VkDescriptorType> &types, const VkDescriptorType &type) {
        std::set<VkDescriptorType>::iterator it;
        for (it = types.begin(); it != types.end(); ++it)
            if (type == *it)
                return true;

        return false;
    }

    DescriptorPool *DescriptorPool::Create(VkDevice device, uint32_t maxSets, std::vector<VkDescriptorPoolSize> sizes) {
        auto&& pool = new DescriptorPool(maxSets);

        pool->m_layout       = VK_NULL_HANDLE;
        pool->m_device       = device;
        pool->m_requestTypes = {};

        auto&& descriptorPoolCI = Tools::Initializers::DescriptorPoolCreateInfo(sizes.size(), sizes.data(), maxSets);

        /// этот флаг позволяет осовбождать сеты дескрипторов по отдельности
        descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &pool->m_pool);
        if (vkRes != VK_SUCCESS) {
            VK_ERROR("DescriptorPool::Create() : failed to create vulkan descriptor pool!");
            return VK_NULL_HANDLE;
        }

        return pool;
    }

    DescriptorPool* DescriptorPool::Create(VkDevice device, uint32_t maxSets, VkDescriptorSetLayout layout, const RequestTypes& requestTypes) {
        if (requestTypes.empty()) {
            VK_ERROR("DescriptorPool::Create() : request types is empty!");
            return nullptr;
        }

        auto&& pool = new DescriptorPool(maxSets);

        pool->m_layout       = layout;
        pool->m_device       = device;
        pool->m_requestTypes = requestTypes;

        std::vector<VkDescriptorPoolSize> sizes = {};
        sizes.reserve(pool->m_poolSizes.sizes.size());
        for (auto&& [type, multiplier] : pool->m_poolSizes.sizes) {
            if (Contains(requestTypes, type)) {
                sizes.push_back({ type, static_cast<uint32_t>(multiplier * maxSets) });
            }
        }

        auto&& descriptorPoolCI = Tools::Initializers::DescriptorPoolCreateInfo(sizes.size(), sizes.data(), maxSets);

        /// этот флаг позволяет осовбождать сеты дескрипторов по отдельности
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

    bool DescriptorPool::IsOutOfMemory() const {
        return m_used >= m_maxSets || m_outOfMemory;
    }

    VkResult DescriptorPool::Free(VkDescriptorSet set) {
        if (m_used != 0) {
            --m_used;
            m_outOfMemory = false;
        }
        else {
            VK_ASSERT2(false, "usage count are zero!");
        }

        return vkFreeDescriptorSets(m_device, m_pool, 1, &set);
    }

    std::pair<VkResult, DescriptorSet> DescriptorPool::Allocate() {
        if (m_used >= m_maxSets) {
            m_outOfMemory = true;
            VK_ERROR("DescriptorPool::Allocate() : descriptor pool overflow!");
            return std::make_pair(VK_ERROR_OUT_OF_POOL_MEMORY, DescriptorSet());
        }

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        auto&& descriptorSetAllocInfo = Tools::Initializers::DescriptorSetAllocateInfo(m_pool, &m_layout, 1);
        auto&& result = vkAllocateDescriptorSets(m_device, &descriptorSetAllocInfo, &descriptorSet);

        switch (result) {
            case VK_SUCCESS:
                ++m_used;
                break;
            case VK_ERROR_FRAGMENTED_POOL:
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                m_outOfMemory = true;
                EVK_FALLTHROUGH;
            default:
                break;
        }

        return std::make_pair(
                result,
                DescriptorSet(descriptorSet, m_layout, this)
        );
    }
}