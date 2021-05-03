//
// Created by Nikita on 12.04.2021.
//

#include "EvoVulkan/Types/Swapchain.h"

#include <EvoVulkan/Tools/VulkanDebug.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>

EvoVulkan::Types::Swapchain* EvoVulkan::Types::Swapchain::Create(
        VkInstance const &instance,
        EvoVulkan::Types::Surface *surface,
        EvoVulkan::Types::Device *device,
        const CmdBuffer* cmdBuff,
        unsigned int width,
        unsigned int height)
{
    VK_GRAPH("Swapchain::Create() : create vulkan swapchain...");

    if (!surface->Ready()) {
        VK_ERROR("Swapchain::Create() : surface isn't ready!");
        return nullptr;
    }

    auto* swapchain = new Swapchain();
    {
        swapchain->m_instance = instance;
        swapchain->m_device = device;
        swapchain->m_surface = surface;

        swapchain->m_swapchain = VK_NULL_HANDLE;
    }

    if (!swapchain->InitFormats()) {
        Tools::VkDebug::Error("Swapchain::Create() : failed to init depth format!");
        return nullptr;
    }

    if (!swapchain->ReSetup(width, height, cmdBuff)) {
        Tools::VkDebug::Error("Swapchain::Create() : failed to setup swapchain!");
        return nullptr;
    }

    Tools::VkDebug::Graph("Swapchain::Create() : swapchain successfully created!");

    return swapchain;
}

bool EvoVulkan::Types::Swapchain::ReSetup(unsigned int width, unsigned int height, const CmdBuffer* cmdBuff) {
    Tools::VkDebug::Graph("Swapchain::ReSetup() : re-setup vulkan swapchain...");

    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCaps = {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_device, *m_surface, &surfCaps) != VK_SUCCESS) {
        Tools::VkDebug::Error("Swapchain::ReSetup() : failed get physical device surface capabilities!");
        return false;
    }

    //! TODO: see VS example
    if (surfCaps.currentExtent.width != width || surfCaps.currentExtent.height != height) {
        Tools::VkDebug::Error("Swapchain::ReSize() : swap chain size different!");
        return false;
    }

    VK_GRAPH("Swapchain::ReSetup() : get present mode...");
    this->m_presentMode = Tools::GetPresentMode(*m_device, *m_surface);

    // Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;

    VkSurfaceTransformFlagsKHR preTransform = {};
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        preTransform = surfCaps.currentTransform;

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.pNext                    = NULL;
    swapchainCI.surface                  = *m_surface;
    swapchainCI.minImageCount            = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat              = m_colorFormat;
    swapchainCI.imageColorSpace          = m_colorSpace;
    swapchainCI.imageExtent              = { width, height };
    swapchainCI.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform             = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers         = 1;
    swapchainCI.queueFamilyIndexCount    = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount    = 0;
    swapchainCI.pQueueFamilyIndices      = NULL;
    swapchainCI.presentMode              = m_presentMode;
    swapchainCI.oldSwapchain             = oldSwapchain;
    swapchainCI.clipped                  = true;
    swapchainCI.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    Tools::VkDebug::Graph("Swapchain::ReSetup() : create swapchain struct...");

    if (vkCreateSwapchainKHR(*m_device, &swapchainCI, nullptr, &m_swapchain) != VK_SUCCESS) {
        Tools::VkDebug::Error("Swapchain::ReSetup() : failed to create swapchain!");
        return false;
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    //! Note: destroying the swapchain also cleans up all its associated
    //! presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(*m_device, oldSwapchain, nullptr);

    //todo: images and buffers creation

    Tools::VkDebug::Graph("Swapchain::ReSetup() : swapchain successfully re-configured!");

    return true;
}

void EvoVulkan::Types::Swapchain::Free() {
    Tools::VkDebug::Log("Swapchain::Free() : free swapchain pointer...");
    delete this;
}

void EvoVulkan::Types::Swapchain::Destroy() {
    Tools::VkDebug::Log("Swapchain::Destroy() : destroy vulkan swapchain...");

    if (!IsReady()) {
        Tools::VkDebug::Error("Swapchain::Destroy() : swapchain isn't ready!");
        return;
    }

    vkDestroySwapchainKHR(*m_device, m_swapchain, nullptr);
    this->m_swapchain = VK_NULL_HANDLE;

    this->m_device      = nullptr;
    this->m_surface     = nullptr;
    this->m_presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    this->m_instance    = VK_NULL_HANDLE;
}

bool EvoVulkan::Types::Swapchain::IsReady() const {
    return  m_swapchain != VK_NULL_HANDLE &&
            m_device    != nullptr        &&
            m_instance  != VK_NULL_HANDLE &&
            m_surface   != nullptr        &&

            m_presentMode != VK_PRESENT_MODE_MAX_ENUM_KHR &&
            m_colorFormat != VK_FORMAT_UNDEFINED          &&
            m_colorSpace  != VK_COLOR_SPACE_MAX_ENUM_KHR  &&
            m_depthFormat != VK_FORMAT_UNDEFINED;
}

bool EvoVulkan::Types::Swapchain::InitFormats() {
    // Gather physical device memory properties

    m_depthFormat = Tools::GetDepthFormat(*m_device);
    if (m_depthFormat == VK_FORMAT_UNDEFINED) {
        Tools::VkDebug::Error("Swapchain::InitFormats() : could not find a supported depth format!");
        return false;
    }

    //!=================================================================================================================

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.

    auto formatCount = m_surface->GetCountSurfFmts();
    auto surfFormats = m_surface->GetSurfaceFormats();

    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
        m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    else
        m_colorFormat = surfFormats[0].format;

    m_colorSpace = surfFormats[0].colorSpace;

    if (m_colorFormat == VK_FORMAT_UNDEFINED) {
        Tools::VkDebug::Error("Swapchain::InitFormats() : color format undefined!");
        return false;
    }

    return true;
}

void EvoVulkan::Types::Swapchain::DestroyBuffers() {
    if (m_countImages > 0 && m_swapchainImages && m_device) {
        for (uint32_t i = 0; i < m_countImages; i++)
            vkDestroyImageView(*m_device, m_buffers[i].m_view, nullptr);

        free(m_buffers);
        m_buffers = nullptr;
    }
}

bool EvoVulkan::Types::Swapchain::CreateImages() {
    vkGetSwapchainImagesKHR(*m_device, m_swapchain, &m_countImages, NULL);

    if (m_countImages == 0) {
        Tools::VkDebug::Error("Swapchain::CreateImages() : count swapchain images is zero!");
        return false;
    }

    m_swapchainImages = (VkImage*)malloc(m_countImages * sizeof(VkImage));
    if (!m_swapchainImages) {
        Tools::VkDebug::Error("Swapchain::CreateImages() : failed to alloc images memory!");
        return false;
    }

    auto result = vkGetSwapchainImagesKHR(*m_device, m_swapchain, &m_countImages, m_swapchainImages);
    if (result != VK_SUCCESS) {
        Tools::VkDebug::Error("Swapchain::CreateImages() : failed to get swapchain images!");
        return false;
    }

    return true;
}

bool EvoVulkan::Types::Swapchain::CreateBuffers() {
    m_buffers = (SwapChainBuffer*)malloc(sizeof(SwapChainBuffer) * m_countImages);
    if (!m_buffers) {
        VK_ERROR("Swapchain::Buffers() : failed to alloc buffers memory!");
        return false;
    }

    for (uint32_t i = 0; i < m_countImages; i++) {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext  = NULL;
        colorAttachmentView.format = m_colorFormat;
        colorAttachmentView.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel   = 0;
        colorAttachmentView.subresourceRange.levelCount     = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount     = 1;
        colorAttachmentView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags                           = 0;

        m_buffers[i].m_image = m_swapchainImages[i];

        /*vkTools::setImageLayout(
                cmdBuffer,
                buffers[i].image,
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        colorAttachmentView.image = buffers[i].image;

        err = vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view);
        assert(!err);*/
    }

    return true;
}
