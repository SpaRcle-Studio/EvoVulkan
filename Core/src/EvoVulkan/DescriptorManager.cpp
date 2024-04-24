//
// Created by Nikita on 06.05.2021.
//

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/DescriptorPool.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

namespace EvoVulkan::Core {
    Types::DescriptorSet DescriptorManager::AllocateDescriptorSet(VkDescriptorSetLayout layout, const RequestTypes& requestTypes, bool reallocate) {
        auto&& pFound = reallocate ? nullptr : FindDescriptorPool(layout, requestTypes);
        Types::DescriptorPool* pPool = pFound;

        if (!pPool) {
            /// VK_LOG("DescriptorPool::Create() : create new descriptor pool. Total: " + std::to_string(m_pools.size() + 1));
            pPool = AllocateDescriptorPool(layout, requestTypes);
        }

        if (!pPool) {
            VK_ERROR("DescriptorManager::AllocateDescriptorSets() : failed to allocate descriptor pool!");
            return Types::DescriptorSet();
        }

        auto&& [result, set] = pPool->Allocate();

        switch (result) {
            case VK_SUCCESS:
                /// all good, return
                return set;
            case VK_ERROR_FRAGMENTED_POOL:
                VK_ERROR("DescriptorManager::AllocateDescriptorSet(): descriptor pool is fragmented!");
                return Types::DescriptorSet();
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                if (!reallocate) {
                    VK_ERROR("DescriptorManager::AllocateDescriptorSet(): out of memory descriptor pool! Trying to reallocate pool...");
                    return AllocateDescriptorSet(layout, requestTypes, true);
                }
                else {
                    VK_ERROR("DescriptorManager::AllocateDescriptorSet(): out of memory descriptor pool!");
                }
                EVK_FALLTHROUGH;
            default:
                VK_ERROR("DescriptorManager::AllocateDescriptorSet() : failed to allocate vulkan descriptor set!"
                         "\n\tReason: " + Tools::Convert::result_to_string(result) +
                         "\n\tDescription: " + Tools::Convert::result_to_description(result));
                /// unrecoverable error
                return Types::DescriptorSet();
        }
    }

    void EvoVulkan::Core::DescriptorManager::Reset() {
        VK_INFO("DescriptorManager::Reset() : reset all descriptor pools!");

        for (auto&& pPool : m_pools) {
            delete pPool;
        }

        m_pools.clear();
    }

    bool EvoVulkan::Core::DescriptorManager::FreeDescriptorSet(Types::DescriptorSet* descriptorSet) {
        if (!descriptorSet || !descriptorSet->Valid()) {
            VK_ERROR("DescriptorManager::FreeDescriptorSet() : descriptor set is nullptr!");
            return false;
        }

        /// берем не по ссылке, чтобы случайно не затереть при вызове Reset
        Types::DescriptorPool* pool = descriptorSet->m_pool;

        if (pool->GetLayout() != descriptorSet->m_layout) {
            VK_ERROR("DescriptorManager::FreeDescriptorSet() : layouts are different! Something went wrong!");
            return false;
        }

        if (pool->Free(*descriptorSet) != VK_SUCCESS) {
            VK_ERROR("DescriptorManager::FreeDescriptorSet() : failed to free descriptor set!");
        }

        descriptorSet->Reset();

        if (pool->GetUsageCount() == 0) {
            VK_LOG("DescriptorPool::FreeDescriptorSet() : free empty descriptor pool. Total: " + std::to_string(m_pools.size() - 1));
            m_pools.erase(pool);
            delete pool;
        }

        return true;
    }

    void DescriptorManager::Free() {
        VK_LOG("DescriptorManager::Free() : free descriptor manager pointer...");

        if (!m_pools.empty()) {
            std::string str;
            uint32_t index = 0;
            for (const auto& pool : m_pools) {
                str += "\n\t[" + std::to_string(index) + "] = " + std::to_string(pool->GetUsageCount()) + " descriptor sets";
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

    Types::DescriptorPool *DescriptorManager::AllocateDescriptorPool(VkDescriptorSetLayout layout, const RequestTypes &requestTypes) {
        auto&& pool = Types::DescriptorPool::Create(*m_device, 1000, layout, requestTypes);

        if (pool) {
            m_pools.insert(pool);
        }

        return pool;
    }

    Types::DescriptorPool *DescriptorManager::FindDescriptorPool(VkDescriptorSetLayout layout, const RequestTypes &requestTypes) {
        for (auto&& pool : m_pools) {
            if (!pool->IsOutOfMemory() && layout == pool->GetLayout() && pool->Equal(requestTypes)) {
                return pool;
            }
        }

        return nullptr;
    }
}