//
// Created by Nikita on 03.05.2021.
//

#ifndef EVOVULKAN_CMDBUFFER_H
#define EVOVULKAN_CMDBUFFER_H

#include <EvoVulkan/Tools/VulkanHelper.h>
#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class CmdPool;
    class Device;

    class DLL_EVK_EXPORT CmdBuffer : public IVkObject {
    private:
        CmdBuffer()  = default;

    public:
        ~CmdBuffer() override;

        operator VkCommandBuffer() const { return m_buffer; }

    public:
        static VkCommandBuffer CreateSimple(const Device* device, const CmdPool* cmdPool, const VkCommandBufferLevel& level);
        static bool ExecuteSingleTime(const Device* device, const CmdPool* cmdPool, const std::function<bool(CmdBuffer*)>& fun);
        static CmdBuffer* BeginSingleTime(const Device* device, const CmdPool* cmdPool);
        static CmdBuffer* Create(const Device* device, const CmdPool* cmdPool, VkCommandBufferLevel level);
        static CmdBuffer* Create(const Device* device, const CmdPool* cmdPool, VkCommandBufferAllocateInfo cmdBufAllocateInfo);

    public:
        bool Begin(const VkCommandBufferUsageFlagBits& usage);
        bool End();

        bool ReAlloc();

        EVK_NODISCARD bool IsBegin() const { return m_isBegin; }

        EVK_NODISCARD bool IsComplete() const override;
        EVK_NODISCARD bool IsReady() const override;

    private:
        bool                        m_isBegin       = false;
        VkCommandBuffer             m_buffer        = VK_NULL_HANDLE;
        const Device*               m_device        = nullptr;
        const CmdPool*              m_cmdPool       = nullptr;
        VkCommandBufferAllocateInfo m_buffAllocInfo = {};

    };
}

#endif //EVOVULKAN_CMDBUFFER_H
