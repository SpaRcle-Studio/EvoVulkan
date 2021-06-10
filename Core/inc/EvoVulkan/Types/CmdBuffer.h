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
        static VkCommandBuffer CreateSimple(const Device* device,
                                            const CmdPool* cmdPool,
                                            const VkCommandBufferLevel& level);

        static CmdBuffer* BeginSingleTime(
                const Device* device,
                const CmdPool* cmdPool);

        static CmdBuffer* Create(
                const Device* device,
                const CmdPool* cmdPool,
                VkCommandBufferLevel level);

        static CmdBuffer* Create(
                const Device* device,
                const CmdPool* cmdPool,
                VkCommandBufferAllocateInfo cmdBufAllocateInfo);
    public:
        inline bool Begin(const VkCommandBufferUsageFlagBits& usage) {
            if (!IsReady()) {
                VK_ERROR("CmdBuffer::Begin() : command buffer isn't ready!");
                return false;
            }

            VkCommandBufferBeginInfo cmdBufInfo = {};
            cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBufInfo.flags = usage;

            auto result = vkBeginCommandBuffer(m_buffer, &cmdBufInfo);
            if (result != VK_SUCCESS) {
                VK_ERROR("CmdBuffer::Begin() : failed to begin command buffer!");
                return false;
            }

            this->m_isBegin = true;

            return true;
        }

        bool End();

        bool ReAlloc();

        [[nodiscard]] bool IsBegin()    const { return m_isBegin; }

        [[nodiscard]] bool IsComplete() const override;
        [[nodiscard]] bool IsReady()    const override;

        void Destroy() override;
        void Free()    override;
    };
}

#endif //EVOVULKAN_CMDBUFFER_H
