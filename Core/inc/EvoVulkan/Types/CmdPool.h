//
// Created by Nikita on 02.05.2021.
//

#ifndef EVOVULKAN_CMDPOOL_H
#define EVOVULKAN_CMDPOOL_H

#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Device;

    class DLL_EVK_EXPORT CmdPool : public IVkObject {
    private:
        CmdPool() = default;
        ~CmdPool() override = default;

    public:
        operator VkCommandPool() const { return m_pool; }

    public:
        static CmdPool* Create(Device* device);

    public:
        void Destroy() override;
        void Free() override;

        EVK_NODISCARD bool IsReady() const override;

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;
        Device* m_device = nullptr;

    };
}

#endif //EVOVULKAN_CMDPOOL_H
