//
// Created by Monika on 26.05.2022.
//

#ifndef EVOVULKAN_DESCRIPTORSET_H
#define EVOVULKAN_DESCRIPTORSET_H

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Types {
    class DescriptorPool;

    struct DLL_EVK_EXPORT DescriptorSet {
        DescriptorSet()
            : DescriptorSet(VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr)
        { }

        /// чтобы можно было инициализировать через nullptr
        DescriptorSet(void* /** null */)
            : DescriptorSet(VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr)
        { }

        DescriptorSet(VkDescriptorSet set, VkDescriptorSetLayout layout, DescriptorPool* pool)
            : m_self(set)
            , m_layout(layout)
            , m_pool(pool)
        { }

        void Reset() noexcept {
            m_self = VK_NULL_HANDLE;
            m_layout = VK_NULL_HANDLE;
            m_pool = nullptr;
        }

        EVK_NODISCARD bool Valid() const noexcept {
            return m_self != VK_NULL_HANDLE && m_layout != VK_NULL_HANDLE && m_pool;
        }

        VkDescriptorSet       m_self;
        VkDescriptorSetLayout m_layout;
        DescriptorPool*       m_pool;

        operator VkDescriptorSet() const { return m_self; }
    };
}

#endif //EVOVULKAN_DESCRIPTORSET_H
