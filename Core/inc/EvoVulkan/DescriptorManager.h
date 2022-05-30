//
// Created by Nikita on 06.05.2021.
//

#ifndef EVOVULKAN_DESCRIPTORMANAGER_H
#define EVOVULKAN_DESCRIPTORMANAGER_H

#include <EvoVulkan/DescriptorSet.h>

namespace EvoVulkan::Types {
    class Device;
}

namespace EvoVulkan::Core {
    class DLL_EVK_EXPORT DescriptorPool : public Tools::NonCopyable {
    public:
        explicit DescriptorPool(uint32_t maxSets)
                : m_maxSets(maxSets)
        { }

        ~DescriptorPool() override;

        operator VkDescriptorPool() const { return m_pool; }

    public:
        static DescriptorPool* Create(Types::Device* device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
        static DescriptorPool* Create(uint32_t maxSets, VkDescriptorSetLayout layout, VkDevice device, const std::set<VkDescriptorType>& requestTypes);

        static bool Contains(const std::set<VkDescriptorType>& types, const VkDescriptorType& type);

    public:
        bool Equal(const std::set<VkDescriptorType>& requestTypes);

        EVK_NODISCARD int64_t FindFree() const;

    public:
        std::set<VkDescriptorType> m_requestTypes   = std::set<VkDescriptorType>();

        /// for check equal alloc request (reference)
        VkDescriptorSetLayout      m_layout         = VK_NULL_HANDLE;

        VkDevice                   m_device         = VK_NULL_HANDLE;
        VkDescriptorPool           m_pool           = VK_NULL_HANDLE;

        uint32_t                   m_used           = 0;
        const uint32_t             m_maxSets        = 0;

        VkDescriptorSet*           m_descriptorSets = nullptr;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class DLL_EVK_EXPORT DescriptorManager : public Tools::NonCopyable {
    private:
        DescriptorManager()  = default;
        ~DescriptorManager() override = default;

    public:
        static DescriptorManager* Create(const EvoVulkan::Types::Device* device);

        void Free();
        void Reset();

        DescriptorSet AllocateDescriptorSets(VkDescriptorSetLayout layout, const std::set<VkDescriptorType>& requestTypes);
        bool FreeDescriptorSet(Core::DescriptorSet descriptorSet);

    private:
        uint32_t                            m_countDescriptorsAllocate = 1000;
        const EvoVulkan::Types::Device*     m_device                   = nullptr;
        std::unordered_set<DescriptorPool*> m_pools                    = std::unordered_set<DescriptorPool*>();

    };
}

#endif //EVOVULKAN_DESCRIPTORMANAGER_H
