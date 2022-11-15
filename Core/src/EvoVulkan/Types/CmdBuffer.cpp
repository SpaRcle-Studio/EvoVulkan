//
// Created by Nikita on 03.05.2021.
//

#include <EvoVulkan/Types/CmdBuffer.h>

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/CmdPool.h>

EvoVulkan::Types::CmdBuffer* EvoVulkan::Types::CmdBuffer::Create(
        const EvoVulkan::Types::Device *device,
        const EvoVulkan::Types::CmdPool *cmdPool,
        VkCommandBufferLevel level)
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = Tools::Initializers::CommandBufferAllocateInfo(*cmdPool, level, 1);
    return Create(device, cmdPool, cmdBufAllocateInfo);
}

EvoVulkan::Types::CmdBuffer* EvoVulkan::Types::CmdBuffer::Create(
        const EvoVulkan::Types::Device *device,
        const EvoVulkan::Types::CmdPool *cmdPool,
        VkCommandBufferAllocateInfo cmdBufAllocateInfo)
{
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

void EvoVulkan::Types::CmdBuffer::Destroy() {
    if (!IsReady()) {
        VK_ERROR("CmdBuffer::Destroy() : command buffer isn't ready!");
        return;
    }

    vkFreeCommandBuffers(*m_device, *m_cmdPool, 1, &m_buffer);
    m_buffer = VK_NULL_HANDLE;

    this->m_device  = nullptr;
    this->m_cmdPool = nullptr;

    this->m_isBegin = false;
}

void EvoVulkan::Types::CmdBuffer::Free() {
    delete this;
}

bool EvoVulkan::Types::CmdBuffer::IsComplete() const{
    return m_device &&
           m_device->IsReady() &&
           m_cmdPool != VK_NULL_HANDLE &&
           m_buffAllocInfo.commandPool == *m_cmdPool;
}

[[nodiscard]] bool EvoVulkan::Types::CmdBuffer::IsReady() const {
    return m_buffer != VK_NULL_HANDLE && IsComplete();
}

EvoVulkan::Types::CmdBuffer* EvoVulkan::Types::CmdBuffer::BeginSingleTime(const Device *device, const CmdPool *cmdPool) {
    auto buffer = Create(device, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    if (!buffer) {
        VK_ERROR("CmdBuffer::BeginSingleTime() : failed to create command buffer!");
        return nullptr;
    }
    if (!buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
        VK_ERROR("CmdBuffer::BeginSingleTime() : failed to begin command buffer!");
        buffer->Destroy();
        buffer->Free();
        return nullptr;
    }
    return buffer;
}

bool EvoVulkan::Types::CmdBuffer::End() {
    this->m_isBegin = false;

    vkEndCommandBuffer(this->m_buffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_buffer;

    auto result = vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        VK_ERROR("CmdBuffer::End() : failed to queue submit!");
        return false;
    }
    vkQueueWaitIdle(m_device->GetGraphicsQueue());

    return true;
}

VkCommandBuffer EvoVulkan::Types::CmdBuffer::CreateSimple(const Device* device,
                                                          const CmdPool* cmdPool,
                                                          const VkCommandBufferLevel& level) {
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = Tools::Initializers::CommandBufferAllocateInfo(*cmdPool, level, 1);
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(*device, &cmdBufAllocateInfo, &cmdBuffer) != VK_SUCCESS) {
        VK_ERROR("CmdBuffer::CreateSimple() : failed to allocate vulkan command buffer!");
        return VK_NULL_HANDLE;
    }
    else
        return cmdBuffer;
}

bool EvoVulkan::Types::CmdBuffer::Begin(const VkCommandBufferUsageFlagBits &usage) {
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

bool EvoVulkan::Types::CmdBuffer::ExecuteSingleTime(const EvoVulkan::Types::Device *device,
                                                    const EvoVulkan::Types::CmdPool *cmdPool,
                                                    const std::function<bool(CmdBuffer * )> &fun)
{
    auto&& cmdBuffer = BeginSingleTime(device, cmdPool);
    if (!cmdBuffer || !fun) {
        return false;
    }

    const bool result = fun(cmdBuffer);

    cmdBuffer->Destroy();
    cmdBuffer->Free();

    return result;
}
