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

    class CmdBuffer : public IVkObject {
    private:
        CmdBuffer()  = default;
        ~CmdBuffer() = default;
    public:
        CmdBuffer(const CmdBuffer&) = delete;
    private:
        VkCommandBuffer             m_buffer        = VK_NULL_HANDLE;
        const Device*               m_device        = nullptr;
        const CmdPool*              m_cmdPool       = nullptr;
        VkCommandBufferAllocateInfo m_buffAllocInfo = {};

        bool                        m_isBegin       = false;
    public:
        operator VkCommandBuffer() const { return m_buffer; }
    public:
        static CmdBuffer* Create(
                const Device* device,
                const CmdPool* cmdPool,
                VkCommandBufferAllocateInfo cmdBufAllocateInfo);

        bool Begin();

        bool ReAlloc();

        [[nodiscard]] bool IsBegin()    const { return m_isBegin; }

        [[nodiscard]] bool IsComplete() const override;
        [[nodiscard]] bool IsReady()    const override;

        void Destroy() override;
        void Free()    override;
    };
}

#endif //EVOVULKAN_CMDBUFFER_H
