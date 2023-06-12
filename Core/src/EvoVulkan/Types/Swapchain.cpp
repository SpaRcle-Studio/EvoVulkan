//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/Types/Swapchain.h>

#include <EvoVulkan/Tools/VulkanHelper.h>
#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>
#include <EvoVulkan/Types/CmdBuffer.h>

#include <EvoVulkan/Tools/VulkanInitializers.h>

EvoVulkan::Types::Swapchain::~Swapchain() {
    VK_LOG("Swapchain::Destroy() : destroy vulkan swapchain...");

    DestroyBuffers();

    if (m_swapchain) {
        vkDestroySwapchainKHR(*m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    m_device      = nullptr;
    m_surface     = nullptr;
    m_presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    m_instance    = VK_NULL_HANDLE;
}

EvoVulkan::Types::Swapchain* EvoVulkan::Types::Swapchain::Create(
        VkInstance const &instance,
        EvoVulkan::Types::Surface *surface,
        EvoVulkan::Types::Device *device,
        bool vsync,
        uint32_t width,
        uint32_t height,
        uint32_t imagesCount)
{
    VK_GRAPH("Swapchain::Create() : create vulkan swapchain...");

    if (!surface->IsReady()) {
        VK_ERROR("Swapchain::Create() : surface isn't ready!");
        return nullptr;
    }

    auto&& pSwapchain = new Swapchain();
    {
        pSwapchain->m_instance  = instance;
        pSwapchain->m_device    = device;
        pSwapchain->m_surface   = surface;

        pSwapchain->m_swapchain = VK_NULL_HANDLE;
        pSwapchain->m_vsync     = vsync;
    }

    if (!pSwapchain->InitFormats()) {
        VK_ERROR("Swapchain::Create() : failed to init depth format!");
        return nullptr;
    }

    if (!pSwapchain->ReSetup(width, height, imagesCount)) {
        VK_ERROR("Swapchain::Create() : failed to setup swapchain!");
        return nullptr;
    }

    VK_GRAPH("Swapchain::Create() : swapchain successfully created!");

    return pSwapchain;
}

bool EvoVulkan::Types::Swapchain::ReSetup(uint32_t width, uint32_t height, uint32_t countImages) {
    VK_GRAPH("Swapchain::ReSetup() : re-setup vulkan swapchain..."
         "\n\tWidth: " + std::to_string(width) + "\n\tHeight: " + std::to_string(height)
     );

    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surfCaps = {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_device, *m_surface, &surfCaps) != VK_SUCCESS) {
        VK_ERROR("Swapchain::ReSetup() : failed get physical device surface capabilities!");
        return false;
    }

    //! TODO: see VS example
    if (surfCaps.currentExtent.width != width || surfCaps.currentExtent.height != height) {
        VK_ASSERT2(false, "Swapchain::ReSize() : swap chain size different! "
                              "\n\tWidth  surface: " + std::to_string(surfCaps.currentExtent.width) +
                              "\n\tHeight surface: " + std::to_string(surfCaps.currentExtent.height) +
                              "\n\tWidth   window: " + std::to_string(width) +
                              "\n\tHeight  window: " + std::to_string(height));
        return false;
    }

    m_surfaceWidth  = surfCaps.currentExtent.width;
    m_surfaceHeight = surfCaps.currentExtent.height;

    if (m_surfaceWidth == 0 || m_surfaceHeight == 0) {
        VK_ERROR("Swapchain::ReSetup() : surface size contains zero!");
        return false;
    }

    VK_GRAPH("Swapchain::ReSetup() : get present mode...");
    m_presentMode = Tools::GetPresentMode(*m_device, *m_surface, m_vsync);

    /// Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = EVK_MAX(surfCaps.minImageCount, countImages);
    VkSurfaceTransformFlagsKHR preTransform;
    {
        if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
            desiredNumberOfSwapchainImages = surfCaps.maxImageCount;

        // Find the transformation of the surface
        if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // We prefer a non-rotated transform
        else
            preTransform = surfCaps.currentTransform;
    }

    // Find a supported composite alpha format (not all devices support alpha opaque)
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // Simply select the first composite alpha format available
    const std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (const auto& compositeAlphaFlag : compositeAlphaFlags) {
        if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
            compositeAlpha = compositeAlphaFlag;
            break;
        };
    }

    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface                  = *m_surface;
    swapchainCI.minImageCount            = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat              = m_colorFormat;
    swapchainCI.imageColorSpace          = m_colorSpace;
    swapchainCI.imageExtent              = { m_surfaceWidth, m_surfaceHeight };
    swapchainCI.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform             = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers         = 1;
    swapchainCI.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.queueFamilyIndexCount    = 0;
    swapchainCI.presentMode              = m_presentMode;
    // Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
    swapchainCI.oldSwapchain             = oldSwapchain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchainCI.clipped                  = VK_TRUE;
    swapchainCI.compositeAlpha           = compositeAlpha;

    // Enable transfer source on swap chain images if supported
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // Enable transfer destination on swap chain images if supported
    if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VK_GRAPH("Swapchain::ReSetup() : create swapchain struct...");

    if (vkCreateSwapchainKHR(*m_device, &swapchainCI, nullptr, &m_swapchain) != VK_SUCCESS) {
        VK_ERROR("Swapchain::ReSetup() : failed to create swapchain!");
        return false;
    }

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    //! Note: destroying the swapchain also cleans up all its associated
    //! presentable images once the platform is done with them.
    if (oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(*m_device, oldSwapchain, nullptr);

    //!=================================================================================================================

    VK_GRAPH("Swapchain::ReSetup() : create images...");
    if (m_swapchainImages) { // images data automatic destroy after destroying swapchain
        free(m_swapchainImages);
        m_countImages = 0;
        m_swapchainImages   = nullptr;
    }

    if (!CreateImages()) {
        VK_ERROR("Swapchain::ReSetup() : failed to create images!");
        return false;
    }

    VK_GRAPH("Swapchain::ReSetup() : create buffers...");
    if (m_buffers) {
        DestroyBuffers();
    }

    if (!CreateBuffers()) {
        VK_ERROR("Swapchain::ReSetup() : failed to create buffers!");
        return false;
    }

    VK_GRAPH("Swapchain::ReSetup() : swapchain successfully re-configured!");

    return true;
}

bool EvoVulkan::Types::Swapchain::IsReady() const {
    return m_swapchain != VK_NULL_HANDLE &&
           m_device    != nullptr        &&
           m_instance  != VK_NULL_HANDLE &&
           m_surface   != nullptr        &&

           m_presentMode != VK_PRESENT_MODE_MAX_ENUM_KHR &&
           m_colorFormat != VK_FORMAT_UNDEFINED          &&
           m_colorSpace  != VK_COLOR_SPACE_MAX_ENUM_KHR;
}

bool EvoVulkan::Types::Swapchain::InitFormats() {
    // Gather physical device memory properties

    if (m_device->GetDepthFormat() == VK_FORMAT_UNDEFINED) {
        VK_ERROR("Swapchain::InitFormats() : could not find a supported depth format!");
        return false;
    }

    //!=================================================================================================================

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.

    auto formatCount = m_surface->GetCountSurfFmts();
    auto surfFormats = m_surface->GetSurfaceFormats();

    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
        m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
        m_colorFormat = surfFormats[0].format;

    m_colorSpace = surfFormats[0].colorSpace;

    if (m_colorFormat == VK_FORMAT_UNDEFINED) {
        VK_ERROR("Swapchain::InitFormats() : color format undefined!");
        return false;
    }

    return true;
}

void EvoVulkan::Types::Swapchain::DestroyBuffers() {
    if (m_countImages > 0 && m_swapchainImages && m_device) {
        for (uint32_t i = 0; i < m_countImages; ++i)
            vkDestroyImageView(*m_device, m_buffers[i].m_view, nullptr);

        if (m_buffers) {
            free(m_buffers);
            m_buffers = nullptr;
        }
    }
    else {
        VK_WARN("Swapchain::DestroyBuffers() : failed to destroy swapchain buffers!");
    }
}

bool EvoVulkan::Types::Swapchain::CreateImages() {
    VkResult result = vkGetSwapchainImagesKHR(*m_device, m_swapchain, &m_countImages, NULL);
    if (result != VK_SUCCESS) {
        VK_ERROR("Swapchain::CreateImages() : failed to get swapchain images count!");
        return false;
    }

    if (m_countImages == 0) {
        VK_ERROR("Swapchain::CreateImages() : count swapchain images is zero!");
        return false;
    }

    VK_LOG("Swapchain::CreateImages() : use " + std::to_string(m_countImages) + " images");

    m_swapchainImages = (VkImage*)malloc(m_countImages * sizeof(VkImage));
    if (!m_swapchainImages) {
        VK_ERROR("Swapchain::CreateImages() : failed to alloc images memory!");
        return false;
    }

    result = vkGetSwapchainImagesKHR(*m_device, m_swapchain, &m_countImages, m_swapchainImages);
    if (result != VK_SUCCESS) {
        VK_ERROR("Swapchain::CreateImages() : failed to get swapchain images!"
                 "\n\tReason: " + Tools::Convert::result_to_string(result) +
                 "\n\tDesciption: " + Tools::Convert::result_to_description(result)
        );
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

    for (uint32_t i = 0; i < m_countImages; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = NULL;
        colorAttachmentView.format = m_colorFormat;
        colorAttachmentView.components = {
                //VK_COMPONENT_SWIZZLE_R,
                //VK_COMPONENT_SWIZZLE_G,
                //VK_COMPONENT_SWIZZLE_B,
                //VK_COMPONENT_SWIZZLE_A

                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        m_buffers[i].m_image = m_swapchainImages[i];

        colorAttachmentView.image = m_buffers[i].m_image;

        auto result = vkCreateImageView(*m_device, &colorAttachmentView, nullptr, &m_buffers[i].m_view);
        if (result != VK_SUCCESS) {
            VK_ERROR("Swapchain::CreateBuffers() : failed to create images view! Reason: "
                + Tools::Convert::result_to_description(result));
            return false;
        }
    }

    return true;
}

VkResult EvoVulkan::Types::Swapchain::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex) const {
    /// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
    /// With that we don't have to handle VK_NOT_READY
    return vkAcquireNextImageKHR(*m_device, m_swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

bool EvoVulkan::Types::Swapchain::SurfaceIsAvailable() {
    VkSurfaceCapabilitiesKHR surfCaps = {};

    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*m_device, *m_surface, &surfCaps) != VK_SUCCESS) {
        return false;
    }

    return surfCaps.currentExtent.height != 0 && surfCaps.currentExtent.width;
}

VkResult EvoVulkan::Types::Swapchain::QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) const {
    VkPresentInfoKHR presentInfo = { };
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = NULL;
    presentInfo.swapchainCount   = 1;
    presentInfo.pSwapchains      = &m_swapchain;
    presentInfo.pImageIndices    = &imageIndex;

    /// Check if a wait semaphore has been specified to wait for before presenting the image
    if (waitSemaphore != VK_NULL_HANDLE) {
        presentInfo.pWaitSemaphores    = &waitSemaphore;
        presentInfo.waitSemaphoreCount = 1;
    }

    try {
        return vkQueuePresentKHR(queue, &presentInfo);
    }
    catch (const std::exception& ex) {
        VK_ERROR("Swapchain::QueuePresent() : an exception has been occurred! \n\tMessage: " + std::string(ex.what()));
        return VK_ERROR_UNKNOWN;
    }
}


