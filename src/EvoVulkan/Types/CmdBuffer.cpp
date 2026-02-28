//
// Created by Nikita on 03.05.2021.
//

#include <EvoVulkan/Types/CmdBuffer.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Profile.h>

namespace EvoVulkan::Types {
    EvoVulkan::Types::CmdBuffer::~CmdBuffer() {
        EVK_TRACY_ZONE;
        End();

        if (m_buffer) {
            vkFreeCommandBuffers(*m_device, *m_cmdPool, 1, &m_buffer);
            m_buffer = VK_NULL_HANDLE;

            m_device = nullptr;
            m_cmdPool = nullptr;

            m_isBegin = false;
        }
    }

    CmdBuffer* CmdBuffer::Create(
        const Device* device,
        const CmdPool* cmdPool,
        VkCommandBufferLevel level)
    {
        EVK_TRACY_ZONE;
        VkCommandBufferAllocateInfo cmdBufAllocateInfo = Tools::Initializers::CommandBufferAllocateInfo(*cmdPool, level, 1);
        return Create(device, cmdPool, cmdBufAllocateInfo);
    }

    CmdBuffer* CmdBuffer::Create(
            const Device *device,
            const CmdPool *cmdPool,
            VkCommandBufferAllocateInfo cmdBufAllocateInfo)
    {
        EVK_TRACY_ZONE;
        auto buffer = new CmdBuffer();

        {
            buffer->m_device        = device;
            buffer->m_cmdPool       = cmdPool;
            buffer->m_buffAllocInfo = cmdBufAllocateInfo;
            buffer->m_buffer        = VK_NULL_HANDLE;
        }

        if (!buffer->ReAlloc()) {
            VK_ERROR("CmdBuffer::Create() : failed to re alloc vulkan command buffer!");
            return nullptr;
        }

        return buffer;
    }

    bool EvoVulkan::Types::CmdBuffer::ReAlloc() {
        EVK_TRACY_ZONE;
        if (!IsComplete()) {
            VK_ERROR("CmdBuffer::ReAlloc() : command buffer isn't complete!");
            return false;
        }

        if (m_buffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(*m_device, *m_cmdPool, 1, &m_buffer);
            m_buffer = VK_NULL_HANDLE;
        }

        VkResult vkRes = vkAllocateCommandBuffers(*m_device, &m_buffAllocInfo, &m_buffer);
        if (vkRes != VK_SUCCESS) {
            VK_ERROR("CmdBuffer::ReAlloc() : failed to allocate vulkan command buffer!");
            return false;
        }

        return true;
    }

    bool CmdBuffer::IsComplete() const{
        return m_device &&
               m_device->IsReady() &&
               m_cmdPool != VK_NULL_HANDLE &&
               m_buffAllocInfo.commandPool == *m_cmdPool;
    }

    bool CmdBuffer::IsReady() const {
        return m_buffer != VK_NULL_HANDLE && IsComplete();
    }

    CmdBuffer* CmdBuffer::BeginSingleTime(const Device *device, const CmdPool *cmdPool) {
        EVK_TRACY_ZONE;

        auto&& pBuffer = Create(device, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (!pBuffer) {
            VK_ERROR("CmdBuffer::BeginSingleTime() : failed to create command buffer!");
            return nullptr;
        }

        pBuffer->m_singleTime = true;

        if (!pBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
            VK_ERROR("CmdBuffer::BeginSingleTime() : failed to begin command buffer!");
            delete pBuffer;
            return nullptr;
        }

        return pBuffer;
    }

    bool CmdBuffer::End(bool doExecute) {
        EVK_TRACY_ZONE;

        if (!m_isBegin) {
            return false;
        }
        m_isBegin = false;

        vkEndCommandBuffer(m_buffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_buffer;

        if (doExecute) {
            {
                EVK_TRACY_ZONE_N("vkQueueSubmit");
                auto result = vkQueueSubmit(m_device->GetQueues()->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                if (result != VK_SUCCESS) {
                    VK_HALT("CmdBuffer::End() : failed to queue submit!");
                    return false;
                }
            }

            m_device->WaitGraphicsQueueIdle();
        }

        return true;
    }

    bool CmdBuffer::Begin(const VkCommandBufferUsageFlagBits &usage) {
        EVK_TRACY_ZONE;

        if (m_isBegin) {
            VK_HALT("CmdBuffer::Begin() : command buffer already begun!");
            return false;
        }

        if (m_singleUsed && m_singleTime) {
            VK_HALT("CmdBuffer::Begin() : single time command buffer can be used only once!");
            return false;
        }
        m_singleUsed = true;

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

        m_isBegin = true;

        return true;
    }

    bool CmdBuffer::ExecuteSingleTime(
        const Device *device,
        const CmdPool *cmdPool,
        const std::function<bool(CmdBuffer*)> &fun
    ) {
        EVK_TRACY_ZONE;

        auto&& cmdBuffer = BeginSingleTime(device, cmdPool);
        if (!cmdBuffer || !fun) {
            return false;
        }

        const bool result = fun(cmdBuffer);

        delete cmdBuffer;

        return result;
    }
}