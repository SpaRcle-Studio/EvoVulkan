//
// Created by Monika on 23.06.2022.
//

#ifndef EVOVULKAN_DESCRIPTORPOOL_H
#define EVOVULKAN_DESCRIPTORPOOL_H

#include <EvoVulkan/Types/DescriptorSet.h>

namespace EvoVulkan::Types {
    class Device;

    struct PoolSizes {
        std::vector<std::pair<VkDescriptorType, float>> sizes =
        {
                { VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f  },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.f  },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.f  },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.f  },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.f  },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4.f  },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.f  },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f  },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f  },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f }
        };
    };

    class DLL_EVK_EXPORT DescriptorPool : public Tools::NonCopyable {
        using RequestTypes = std::set<VkDescriptorType>;
    public:
        explicit DescriptorPool(uint32_t maxSets)
            : m_maxSets(maxSets)
        { }

        ~DescriptorPool() override;

        operator VkDescriptorPool() const { return m_pool; }

    public:
        static DescriptorPool* Create(VkDevice device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& sizes);
        static DescriptorPool* Create(VkDevice device, uint32_t maxSets, VkDescriptorSetLayout layout, const RequestTypes& requestTypes);
        static bool Contains(const std::set<VkDescriptorType>& types, const VkDescriptorType& type);

    public:
        std::pair<VkResult, DescriptorSet> Allocate();
        VkResult Free(VkDescriptorSet set);

        bool Equal(const std::set<VkDescriptorType>& requestTypes);

        EVK_NODISCARD bool IsOutOfMemory() const;
        EVK_NODISCARD uint32_t GetUsageCount() const { return m_used; }
        EVK_NODISCARD VkDescriptorSetLayout GetLayout() const { return m_layout; }

    private:
        bool Initialize(const std::vector<VkDescriptorPoolSize>& sizes);

    private:
        std::set<VkDescriptorType> m_requestTypes   = std::set<VkDescriptorType>();
        PoolSizes                  m_poolSizes      = PoolSizes();

        /// for check equal alloc request (reference)
        VkDescriptorSetLayout      m_layout         = VK_NULL_HANDLE;

        VkDevice                   m_device         = VK_NULL_HANDLE;
        VkDescriptorPool           m_pool           = VK_NULL_HANDLE;

        uint32_t                   m_used           = 0;
        const uint32_t             m_maxSets        = 0;
        bool                       m_outOfMemory    = false;

    };
}

#endif //SRENGINE_DESCRIPTORPOOL_H
