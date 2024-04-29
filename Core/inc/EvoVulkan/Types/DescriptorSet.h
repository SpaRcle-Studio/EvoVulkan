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

        DescriptorSet(VkDescriptorSet descriptorSet, VkDescriptorSetLayout layout, DescriptorPool* pPool)
            : descriptorSet(descriptorSet)
            , layout(layout)
            , pPool(pPool)
        { }

        void Reset() noexcept {
            descriptorSet = VK_NULL_HANDLE;
            layout = VK_NULL_HANDLE;
            pPool = nullptr;
        }

        EVK_NODISCARD bool Valid() const noexcept {
            return descriptorSet != VK_NULL_HANDLE && layout != VK_NULL_HANDLE && pPool;
        }

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        DescriptorPool* pPool = nullptr;

        operator VkDescriptorSet() const { return descriptorSet; }
    };
}

#endif //EVOVULKAN_DESCRIPTORSET_H
