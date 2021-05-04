//
// Created by Nikita on 04.05.2021.
//

#include <EvoVulkan/Types/DepthStencil.h>
#include <EvoVulkan/Tools/VulkanDebug.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Swapchain.h>

EvoVulkan::Types::DepthStencil* EvoVulkan::Types::DepthStencil::Create(const EvoVulkan::Types::Device *device,
                                                                       const EvoVulkan::Types::Swapchain *swapchain,
                                                                       uint32_t width, uint32_t height)
{
    auto depthStencil = new DepthStencil();
    {
        depthStencil->m_device    = device;
        depthStencil->m_swapchain = swapchain;

        depthStencil->m_image     = VK_NULL_HANDLE;
        depthStencil->m_mem       = VK_NULL_HANDLE;
        depthStencil->m_view      = VK_NULL_HANDLE;
    }

    if (!depthStencil->ReCreate(width, height)) {
        VK_ERROR("DepthStencil::Create() : failed to re-create depth stencil!");
        return nullptr;
    }

    return depthStencil;
}

bool EvoVulkan::Types::DepthStencil::ReCreate(uint32_t width, uint32_t height) {
    VK_GRAPH("DepthStencil::ReCreate() : re-create vulkan depth stencil...");

    if (m_view != VK_NULL_HANDLE)
        vkDestroyImageView(*m_device, m_view, nullptr);
    if (m_image != VK_NULL_HANDLE)
        vkDestroyImage(*m_device, m_image, nullptr);
    if (m_mem != VK_NULL_HANDLE)
        vkFreeMemory(*m_device, m_mem, nullptr);

    VkImageCreateInfo imageCI = {};
    imageCI.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType         = VK_IMAGE_TYPE_2D;
    imageCI.format            = m_swapchain->GetDepthFormat();
    imageCI.extent            = { width, height, 1 };
    imageCI.mipLevels         = 1;
    imageCI.arrayLayers       = 1;
    imageCI.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    auto result = vkCreateImage(*m_device, &imageCI, nullptr, &m_image);
    if (result != VK_SUCCESS) {
        VK_ERROR("DepthStencil::ReCreate() : failed to create vulkan image! Reason: "
            + Tools::Convert::result_to_description(result));
        return false;
    }
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(*m_device, m_image, &memReqs);

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize       = memReqs.size;
    memAlloc.memoryTypeIndex      = m_device->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    result = vkAllocateMemory(*m_device, &memAlloc, nullptr, &m_mem);
    if (result != VK_SUCCESS) {
        VK_ERROR("DepthStencil::ReCreate() : failed to allocate memory! Reason: "
                 + Tools::Convert::result_to_description(result));
        return false;
    }

    result = vkBindImageMemory(*m_device, m_image, m_mem, 0);
    if (result != VK_SUCCESS) {
        VK_ERROR("DepthStencil::ReCreate() : failed to bind image! Reason: "
                 + Tools::Convert::result_to_description(result));
        return false;
    }

    VkImageViewCreateInfo imageViewCI           = {};
    imageViewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image                           = m_image;
    imageViewCI.format                          = m_swapchain->GetDepthFormat();
    imageViewCI.subresourceRange.baseMipLevel   = 0;
    imageViewCI.subresourceRange.levelCount     = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount     = 1;
    imageViewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (m_swapchain->GetDepthFormat() >= VK_FORMAT_D16_UNORM_S8_UINT)
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

    result = vkCreateImageView(*m_device, &imageViewCI, nullptr, &m_view);
    if (result != VK_SUCCESS) {
        VK_ERROR("DepthStencil::ReCreate() : failed to create image view! Reason: "
                 + Tools::Convert::result_to_description(result));
        return false;
    }

    return true;
}

bool EvoVulkan::Types::DepthStencil::IsReady() const {
    return m_image != VK_NULL_HANDLE &&
           m_view != VK_NULL_HANDLE &&
           m_mem != VK_NULL_HANDLE &&
           m_device && m_swapchain;
}

void EvoVulkan::Types::DepthStencil::Destroy() {
    VK_LOG("DepthStencil::Destroy() : destroy depth stencil...");

    if (!IsReady()) {
        VK_ERROR("DepthStencil::Destroy() : depth stencil isn't ready!");
        return;
    }

    vkDestroyImageView(*m_device, m_view, nullptr);
    vkDestroyImage(*m_device, m_image, nullptr);
    vkFreeMemory(*m_device, m_mem, nullptr);

    m_view  = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_mem   = VK_NULL_HANDLE;

    this->m_swapchain = nullptr;
    this->m_device    = nullptr;
}

void EvoVulkan::Types::DepthStencil::Free() {
    VK_LOG("DepthStencil::Free() : free depth stencil pointer...");
    delete this;
}
