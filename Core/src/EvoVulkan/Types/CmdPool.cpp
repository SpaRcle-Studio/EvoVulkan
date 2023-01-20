//
// Created by Nikita on 02.05.2021.
//

#include <EvoVulkan/Types/CmdPool.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Tools/VulkanConverter.h>

EvoVulkan::Types::CmdPool::~CmdPool() {
    VK_LOG("CmdPool::Destroy() : destroy command pool...");

    if (m_pool) {
        vkDestroyCommandPool(*m_device, m_pool, nullptr);

        m_device = nullptr;
        m_pool = VK_NULL_HANDLE;
    }
}

bool EvoVulkan::Types::CmdPool::IsReady() const {
    return m_device && m_pool != VK_NULL_HANDLE;
}

EvoVulkan::Types::CmdPool *EvoVulkan::Types::CmdPool::Create(EvoVulkan::Types::Device *device) {
    VK_GRAPH("CmdPool::Create() : create vulkan command pool...");

    if (!device->IsReady()) {
        VK_ERROR("CmdPool::Create() : device isn't ready!");
        return nullptr;
    }

    VkCommandPool cmdPool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex        = device->GetQueues()->GetGraphicsIndex();
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult vkRes = vkCreateCommandPool(*device, &cmdPoolInfo, nullptr, &cmdPool);
    if (vkRes != VK_SUCCESS) {
        VK_ERROR("CmdPool::CreateCmd() : failed to create command pool! Reason: "
            + Tools::Convert::result_to_description(vkRes));
        return nullptr;
    }

    auto* commandPool = new CmdPool();
    {
        commandPool->m_pool   = cmdPool;
        commandPool->m_device = device;
    }

    return commandPool;
}
