//
// Created by Nikita on 06.05.2021.
//

#ifndef EVOVULKAN_DESCRIPTORMANAGER_H
#define EVOVULKAN_DESCRIPTORMANAGER_H

#include <EvoVulkan/Types/DescriptorSet.h>

namespace EvoVulkan::Types {
    class Device;
    class DescriptorPool;
}

namespace EvoVulkan::Core {
    class DLL_EVK_EXPORT DescriptorManager : public Tools::NonCopyable {
        using RequestTypes = std::vector<uint64_t>;
    private:
        DescriptorManager() = default;
        ~DescriptorManager() override = default;

    public:
        static DescriptorManager* Create(const EvoVulkan::Types::Device* device);

        void Free();
        void Reset();

        Types::DescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout, const RequestTypes& requestTypes, bool reallocate = false);
        bool FreeDescriptorSet(Types::DescriptorSet* descriptorSet);

    private:
        Types::DescriptorPool* FindDescriptorPool(VkDescriptorSetLayout layout, const RequestTypes& requestTypes);
        Types::DescriptorPool* AllocateDescriptorPool(VkDescriptorSetLayout layout, const RequestTypes& requestTypes);

    private:
        const EvoVulkan::Types::Device* m_device = nullptr;
        std::set<Types::DescriptorPool*> m_pools;

    };
}

#endif //EVOVULKAN_DESCRIPTORMANAGER_H
