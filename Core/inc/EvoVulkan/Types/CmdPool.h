//
// Created by Nikita on 02.05.2021.
//

#ifndef EVOVULKAN_CMDPOOL_H
#define EVOVULKAN_CMDPOOL_H

#include <vulkan/vulkan.h>

#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Device;

    class CmdPool : public IVkObject {
    public:
        CmdPool(const CmdPool&) = delete;
    private:
        CmdPool()  = default;
        ~CmdPool() = default;
    private:
        VkCommandPool m_pool   = VK_NULL_HANDLE;
        Device*       m_device = nullptr;
    public:
        static CmdPool* Create(Device* device);
    public:
        operator VkCommandPool() const { return m_pool; }
    public:
        [[nodiscard]] bool IsReady() const override;
        void Destroy() override;
        void Free()    override;
    };
}

#endif //EVOVULKAN_CMDPOOL_H
