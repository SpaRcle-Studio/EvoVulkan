//
// Created by Monika on 23.06.2022.
//

#include <EvoVulkan/Types/DescriptorPool.h>
#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Types {
    DescriptorPool::~DescriptorPool()  {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(*m_pDevice, m_pool, nullptr);
            m_pool = VK_NULL_HANDLE;
        }
    }

    bool DescriptorPool::Contains(const RequestTypes &types, const VkDescriptorType &type) {
        for (auto&& available : types) {
            if (available == static_cast<uint64_t>(type)) {
                return true;
            }
        }

        //RequestTypes::iterator it;
        //for (it = types.begin(); it != types.end(); ++it)
        //    if (type == *it)
        //        return true;

        return false;
    }

    DescriptorPool *DescriptorPool::Create(const Device* pDevice, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& sizes) {
        auto&& pool = new DescriptorPool(maxSets);

        pool->m_layout = VK_NULL_HANDLE;
        pool->m_pDevice = pDevice;
        pool->m_requestTypes = {};

        if (pool->Initialize(sizes)) {
            return pool;
        }

        VK_ERROR("DescriptorPool::Create() : failed to initialize descriptor pool!");

        delete pool;
        return nullptr;
    }

    DescriptorPool* DescriptorPool::Create(const Device* pDevice, uint32_t maxSets, VkDescriptorSetLayout layout, const RequestTypes& requestTypes) {
        if (requestTypes.empty()) {
            VK_ERROR("DescriptorPool::Create() : request types is empty!");
            return nullptr;
        }

        auto &&pool = new DescriptorPool(maxSets);

        pool->m_layout = layout;
        pool->m_pDevice = pDevice;
        pool->m_requestTypes = requestTypes;

        std::vector<VkDescriptorPoolSize> sizes = {};
        sizes.reserve(pool->m_poolSizes.sizes.size());
        for (auto&&[type, multiplier] : pool->m_poolSizes.sizes) {
            if (Contains(requestTypes, type)) {
                sizes.push_back({type, static_cast<uint32_t>(multiplier * maxSets)});
            }
        }

        if (pool->Initialize(sizes)) {
            return pool;
        }

        VK_ERROR("DescriptorPool::Create() : failed to initialize descriptor pool!");

        delete pool;
        return nullptr;
    }

    bool DescriptorPool::Initialize(const std::vector<VkDescriptorPoolSize>& sizes) {
        EVK_TRACY_ZONE;

        /// проверим нет ли повторяющихся типов дескрипторов
        for (size_t i = 0; i < sizes.size(); ++i) {
            for (size_t j = i + 1; j < sizes.size(); ++j) {
                if (sizes[i].type == sizes[j].type) {
                    VK_HALT("DescriptorPool::Initialize() : found duplicate descriptor type in pool sizes!");
                    return false;
                }
            }
        }

        auto&& descriptorPoolCI = Tools::Initializers::DescriptorPoolCreateInfo(sizes.size(), sizes.data(), m_maxSets);

        /// этот флаг позволяет осовбождать сеты дескрипторов по отдельности
        descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        {
            EVK_TRACY_ZONE_N("vkCreateDescriptorPool");
            VkResult vkRes = vkCreateDescriptorPool(*m_pDevice, &descriptorPoolCI, nullptr, &m_pool);
            if (vkRes != VK_SUCCESS) {
                VK_ERROR("DescriptorPool::Initialize() : failed to create vulkan descriptor pool!");
                return false;
            }
        }

        if (m_pDevice->IsValidationEnabled()) {
            char buf[64];
            snprintf(buf, sizeof(buf), "0x%016" PRIxPTR, reinterpret_cast<uintptr_t>(m_pool));
            std::string logDescriptorSizes;
            logDescriptorSizes.reserve(sizes.size() * 32);
            for (auto&& size : sizes) {
                logDescriptorSizes += "\n\t* " + Tools::Convert::DescriptorTypeToString(size.type) + ": " + std::to_string(size.descriptorCount);
            }

            VK_LOG("DescriptorPool::Initialize() : descriptor pool " + std::string(buf) + " created successfully! Sizes:" + logDescriptorSizes);
        }

        return true;
    }

    bool DescriptorPool::Equal(const RequestTypes &requestTypes) {
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

        return vkFreeDescriptorSets(*m_pDevice, m_pool, 1, &set);
    }

    std::pair<VkResult, DescriptorSet> DescriptorPool::Allocate() {
        if (m_used >= m_maxSets) {
            m_outOfMemory = true;
            VK_ERROR("DescriptorPool::Allocate() : descriptor pool overflow!");
            return std::make_pair(VK_ERROR_OUT_OF_POOL_MEMORY, DescriptorSet());
        }

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        auto&& descriptorSetAllocInfo = Tools::Initializers::DescriptorSetAllocateInfo(m_pool, &m_layout, 1);
        auto&& result = vkAllocateDescriptorSets(*m_pDevice, &descriptorSetAllocInfo, &descriptorSet);

        switch (result) {
            case VK_SUCCESS: {
                if ((++m_used) >= m_maxSets) {
                    m_outOfMemory = true;
                }
                break;
            }
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