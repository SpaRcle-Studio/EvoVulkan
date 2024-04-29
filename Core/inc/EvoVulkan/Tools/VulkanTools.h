//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANTOOLS_H
#define EVOVULKAN_VULKANTOOLS_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Types/CmdBuffer.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/DepthStencil.h>
#include <EvoVulkan/Types/Synchronization.h>
#include <EvoVulkan/Types/Instance.h>
#include <EvoVulkan/Types/Pipeline.h>

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanHelper.h>

#include <EvoVulkan/Tools/VulkanConverter.h>

#include <EvoVulkan/Types/VulkanBuffer.h>
#include "DeviceTools.h"
#include "EvoVulkan/Types/Instance.h"

namespace EvoVulkan::Tools {
	struct PushConstantRange {
        VkShaderStageFlags stageFlags;
        uint32_t offset;
        uint32_t size;
    };
	
    DLL_EVK_EXPORT VkAttachmentDescription CreateColorAttachmentDescription(VkFormat format,
                                                             VkSampleCountFlagBits samples,
                                                             VkImageLayout init,
                                                             VkImageLayout final);

    DLL_EVK_EXPORT VkShaderModule LoadShaderModule(const char *fileName, VkDevice device);

    DLL_EVK_EXPORT VkPipelineLayout CreatePipelineLayout(const VkDevice& device, uint32_t setLayoutCount, VkDescriptorSetLayout descriptorSetLayout, const std::vector<VkPushConstantRange>& pushConstants);

    DLL_EVK_EXPORT VkDescriptorSetLayout CreateDescriptorLayout(const VkDevice& device, const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings);

    EVK_MAYBE_UNUSED static uint8_t GetPixelTypeSize(VkFormat format) {
        switch (format) {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_USCALED:
            case VK_FORMAT_R8_SSCALED:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_R8_SRGB:
                return sizeof(int8_t);
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_USCALED:
            case VK_FORMAT_R8G8_SSCALED:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R8G8_SRGB:
                return sizeof(int8_t);
            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_USCALED:
            case VK_FORMAT_R8G8B8_SSCALED:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SINT:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_B8G8R8_UNORM:
            case VK_FORMAT_B8G8R8_SNORM:
            case VK_FORMAT_B8G8R8_USCALED:
            case VK_FORMAT_B8G8R8_SSCALED:
            case VK_FORMAT_B8G8R8_UINT:
            case VK_FORMAT_B8G8R8_SINT:
            case VK_FORMAT_B8G8R8_SRGB:
                return sizeof(int8_t);
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_USCALED:
            case VK_FORMAT_R8G8B8A8_SSCALED:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SNORM:
            case VK_FORMAT_B8G8R8A8_USCALED:
            case VK_FORMAT_B8G8R8A8_SSCALED:
            case VK_FORMAT_B8G8R8A8_UINT:
            case VK_FORMAT_B8G8R8A8_SINT:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return sizeof(int8_t);
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SNORM:
            case VK_FORMAT_R16_USCALED:
            case VK_FORMAT_R16_SSCALED:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R16_SFLOAT:
                return sizeof(int16_t);
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SNORM:
            case VK_FORMAT_R16G16_USCALED:
            case VK_FORMAT_R16G16_SSCALED:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
                return sizeof(int16_t);
            case VK_FORMAT_R16G16B16_UNORM:
            case VK_FORMAT_R16G16B16_SNORM:
            case VK_FORMAT_R16G16B16_USCALED:
            case VK_FORMAT_R16G16B16_SSCALED:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SINT:
            case VK_FORMAT_R16G16B16_SFLOAT:
                return sizeof(int16_t);
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SNORM:
            case VK_FORMAT_R16G16B16A16_USCALED:
            case VK_FORMAT_R16G16B16A16_SSCALED:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return sizeof(int16_t);
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_R32_SFLOAT:
                return sizeof(int32_t);
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
                return sizeof(int32_t);
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SINT:
            case VK_FORMAT_R32G32B32_SFLOAT:
                return sizeof(int32_t);
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return sizeof(int32_t);
            case VK_FORMAT_R64_UINT:
            case VK_FORMAT_R64_SINT:
            case VK_FORMAT_R64_SFLOAT:
                return sizeof(int64_t);
            case VK_FORMAT_R64G64_UINT:
            case VK_FORMAT_R64G64_SINT:
            case VK_FORMAT_R64G64_SFLOAT:
                return sizeof(int64_t);
            case VK_FORMAT_R64G64B64_UINT:
            case VK_FORMAT_R64G64B64_SINT:
            case VK_FORMAT_R64G64B64_SFLOAT:
                return sizeof(int64_t);
            case VK_FORMAT_R64G64B64A64_UINT:
            case VK_FORMAT_R64G64B64A64_SINT:
            case VK_FORMAT_R64G64B64A64_SFLOAT:
                return sizeof(int64_t);
            default:
                VK_HALT("Unknown format!");
                return 0;
        }
    }

    EVK_MAYBE_UNUSED static uint8_t GetPixelChannelsCount(VkFormat format) {
        switch (format) {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_USCALED:
            case VK_FORMAT_R8_SSCALED:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_R8_SRGB:
                return 1;
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_USCALED:
            case VK_FORMAT_R8G8_SSCALED:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R8G8_SRGB:
                return 2;
            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_USCALED:
            case VK_FORMAT_R8G8B8_SSCALED:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SINT:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_B8G8R8_UNORM:
            case VK_FORMAT_B8G8R8_SNORM:
            case VK_FORMAT_B8G8R8_USCALED:
            case VK_FORMAT_B8G8R8_SSCALED:
            case VK_FORMAT_B8G8R8_UINT:
            case VK_FORMAT_B8G8R8_SINT:
            case VK_FORMAT_B8G8R8_SRGB:
                return 3;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_USCALED:
            case VK_FORMAT_R8G8B8A8_SSCALED:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SNORM:
            case VK_FORMAT_B8G8R8A8_USCALED:
            case VK_FORMAT_B8G8R8A8_SSCALED:
            case VK_FORMAT_B8G8R8A8_UINT:
            case VK_FORMAT_B8G8R8A8_SINT:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return 4;
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SNORM:
            case VK_FORMAT_R16_USCALED:
            case VK_FORMAT_R16_SSCALED:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R16_SFLOAT:
                return 1;
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SNORM:
            case VK_FORMAT_R16G16_USCALED:
            case VK_FORMAT_R16G16_SSCALED:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
                return 2;
            case VK_FORMAT_R16G16B16_UNORM:
            case VK_FORMAT_R16G16B16_SNORM:
            case VK_FORMAT_R16G16B16_USCALED:
            case VK_FORMAT_R16G16B16_SSCALED:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SINT:
            case VK_FORMAT_R16G16B16_SFLOAT:
                return 3;
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SNORM:
            case VK_FORMAT_R16G16B16A16_USCALED:
            case VK_FORMAT_R16G16B16A16_SSCALED:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 4;
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_R32_SFLOAT:
                return 1;
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
                return 2;
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SINT:
            case VK_FORMAT_R32G32B32_SFLOAT:
                return 3;
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 4;
            case VK_FORMAT_R64_UINT:
            case VK_FORMAT_R64_SINT:
            case VK_FORMAT_R64_SFLOAT:
                return 1;
            case VK_FORMAT_R64G64_UINT:
            case VK_FORMAT_R64G64_SINT:
            case VK_FORMAT_R64G64_SFLOAT:
                return 2;
            case VK_FORMAT_R64G64B64_UINT:
            case VK_FORMAT_R64G64B64_SINT:
            case VK_FORMAT_R64G64B64_SFLOAT:
                return 3;
            case VK_FORMAT_R64G64B64A64_UINT:
            case VK_FORMAT_R64G64B64A64_SINT:
            case VK_FORMAT_R64G64B64A64_SFLOAT:
                return 4;
            default:
                VK_HALT("Unknown format!");
                return 0;
        }
    }

    EVK_MAYBE_UNUSED Types::Pipeline* CreateStandardGeometryPipeLine(
            const Types::Device* device,
            const std::vector<VkDynamicState>& dynamicStateEnables,
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
            VkVertexInputBindingDescription vertexInputBinding,
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
            VkPipelineCache pipelineCache);

    EVK_MAYBE_UNUSED static VkCommandBuffer* AllocateCommandBuffers(const VkDevice& device, VkCommandBufferAllocateInfo allocInfo) {
        auto cmdBuffs = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * allocInfo.commandBufferCount);

        auto result = vkAllocateCommandBuffers(device, &allocInfo, cmdBuffs);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::AllocateCommandBuffers() : failed to allocate vulkan command buffer!");
            return nullptr;
        }

        return cmdBuffs;
    }

    EVK_MAYBE_UNUSED static void FreeCommandBuffers(const VkDevice& device, const VkCommandPool& cmdPool, VkCommandBuffer** cmdBuffs, uint32_t count) {
        if (cmdBuffs && *cmdBuffs) {
            vkFreeCommandBuffers(device, cmdPool, count, *cmdBuffs);

            free(*cmdBuffs);
            *cmdBuffs = nullptr;
        } else
            VK_ERROR("Tools::FreeCommandBuffers() : command buffers in nullptr!");
    }

    EVK_MAYBE_UNUSED void DestroyPipelineCache(const VkDevice& device, VkPipelineCache* cache);

    EVK_MAYBE_UNUSED VkPipelineCache CreatePipelineCache(const VkDevice& device);

    EVK_MAYBE_UNUSED void DestroySynchronization(const VkDevice& device, Types::Synchronization* sync);

    EVK_MAYBE_UNUSED Types::Synchronization CreateSynchronization(const VkDevice& device);

    EVK_MAYBE_UNUSED static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    EVK_MAYBE_UNUSED static VkDebugUtilsMessengerEXT SetupDebugMessenger(const VkInstance& instance) {
        VK_GRAPH("VulkanTools::SetupDebugMessenger() : setup vulkan debug messenger...");

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        Tools::PopulateDebugMessengerCreateInfo(createInfo);

        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        auto result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS) {
            VK_ERROR("VulkanTools::SetupDebugMessenger() : failed to set up debug messenger! Reason: "
                + Tools::Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return debugMessenger;
    }

    EVK_MAYBE_UNUSED static Types::Surface* CreateSurface(const VkInstance& instance, const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate, void* windowHandle) {
        EVK_MAYBE_UNUSED VkSurfaceKHR surfaceKhr = platformCreate(instance);
        if (surfaceKhr == VK_NULL_HANDLE) {
            VK_ERROR("VulkanTools::CreateSurface() : failed platform-create vulkan surface!");
            return nullptr;
        }

        EVK_MAYBE_UNUSED Types::Surface* surface = Types::Surface::Create(
                surfaceKhr,
                instance,
                windowHandle);

        return surface;
    }

    EVK_MAYBE_UNUSED static VkDevice CreateLogicalDevice(
            VkPhysicalDevice physicalDevice,
            Types::FamilyQueues *pQueues,
            const std::vector<const char *> &extensions,
            const std::vector<const char *> &validLayers)
    {
        VK_GRAPH("VulkanTools::CreateLogicalDevice() : create vulkan logical device...");

        //!=============================================================================================================

        std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
        const float priority = 0.0f;

        VkDeviceQueueCreateInfo deviceQueueCreateInfo;
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.pNext = nullptr;
        deviceQueueCreateInfo.flags = 0;
        deviceQueueCreateInfo.queueFamilyIndex = pQueues->GetGraphicsIndex();
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &priority;
        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

        if (pQueues->GetComputeIndex() != pQueues->GetGraphicsIndex()) {
            deviceQueueCreateInfo.queueFamilyIndex = pQueues->GetComputeIndex();
            deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
        }

        if (pQueues->GetTransferIndex() != pQueues->GetGraphicsIndex() && pQueues->GetTransferIndex() != pQueues->GetComputeIndex()) {
            deviceQueueCreateInfo.queueFamilyIndex = pQueues->GetTransferIndex();
            deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
        }

        //!=============================================================================================================

        /// VkPhysicalDeviceImagelessFramebufferFeatures physicalDeviceImagelessFramebufferFeatures = {};
        /// physicalDeviceImagelessFramebufferFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
        /// physicalDeviceImagelessFramebufferFeatures.imagelessFramebuffer = true;

        //!=============================================================================================================

        /// VkPhysicalDeviceShaderAtomicFloatFeaturesEXT floatFeatures;
        /// floatFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
        /// floatFeatures.pNext = nullptr;
        /// floatFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
        /// floatFeatures.shaderBufferFloat32Atomics = VK_TRUE;
        /// floatFeatures.shaderBufferFloat64Atomics = VK_FALSE;
        /// floatFeatures.shaderBufferFloat64AtomicAdd = VK_FALSE;
        /// floatFeatures.shaderSharedFloat32Atomics = VK_FALSE;
        /// floatFeatures.shaderSharedFloat32AtomicAdd = VK_FALSE;
        /// floatFeatures.shaderSharedFloat64Atomics = VK_FALSE;
        /// floatFeatures.shaderSharedFloat64AtomicAdd = VK_FALSE;
        /// floatFeatures.shaderImageFloat32Atomics = VK_FALSE;
        /// floatFeatures.shaderImageFloat32AtomicAdd = VK_FALSE;
        /// floatFeatures.sparseImageFloat32Atomics = VK_FALSE;
        /// floatFeatures.sparseImageFloat32AtomicAdd = VK_FALSE;

        //!=============================================================================================================

        /// VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeaturesKhr = {};
        /// fragmentShadingRateFeaturesKhr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        /// fragmentShadingRateFeaturesKhr.attachmentFragmentShadingRate = VK_TRUE;
        /// fragmentShadingRateFeaturesKhr.primitiveFragmentShadingRate = VK_TRUE;
        /// fragmentShadingRateFeaturesKhr.pipelineFragmentShadingRate = VK_TRUE;
        /// fragmentShadingRateFeaturesKhr.pNext = nullptr;

        //!=============================================================================================================

        /// VkPhysicalDeviceLineRasterizationFeaturesEXT lineRasterizationFeaturesExt = {};
        /// lineRasterizationFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
        /// lineRasterizationFeaturesExt.pNext = (void*)&fragmentShadingRateFeaturesKhr;
        /// lineRasterizationFeaturesExt.rectangularLines         = VK_FALSE;
        /// lineRasterizationFeaturesExt.bresenhamLines           = VK_TRUE;
        /// lineRasterizationFeaturesExt.smoothLines              = VK_FALSE;
        /// lineRasterizationFeaturesExt.stippledRectangularLines = VK_FALSE;
        /// lineRasterizationFeaturesExt.stippledBresenhamLines   = VK_TRUE;
        /// lineRasterizationFeaturesExt.stippledSmoothLines      = VK_FALSE;

        //!=============================================================================================================

        VkPhysicalDeviceVulkan12Features deviceVulkan12Features = { };
        deviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        deviceVulkan12Features.pNext = nullptr;
        /// deviceVulkan12Features.separateDepthStencilLayouts = VK_TRUE;

        //!=============================================================================================================

        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = (void*)&deviceVulkan12Features;
        /// deviceFeatures2.pNext = (void*)&floatFeatures;
        /// deviceFeatures2.pNext = (void*)&lineRasterizationFeaturesExt;

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        deviceFeatures2.features = deviceFeatures;

        //!=============================================================================================================

        VkDeviceCreateInfo createInfo      = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext                   = (void*)&deviceFeatures2;

        createInfo.queueCreateInfoCount    = static_cast<uint32_t>(deviceQueueCreateInfos.size());
        createInfo.pQueueCreateInfos       = deviceQueueCreateInfos.data();

        createInfo.pEnabledFeatures        = VK_FALSE; /// &deviceFeatures;

        createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (validLayers.empty()) {
            VK_GRAPH("VulkanTools::CreateLogicalDevice() : validation layers disabled.");
            createInfo.enabledLayerCount = 0;
        }
        else {
            VK_GRAPH("VulkanTools::CreateLogicalDevice() : validation layers enabled.");

            createInfo.enabledLayerCount   = static_cast<uint32_t>(validLayers.size());
            createInfo.ppEnabledLayerNames = validLayers.data();
        }

        VkDevice device = VK_NULL_HANDLE;
        auto result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            VK_ERROR("VulkanTools::CreateLogicalDevice() : failed to create logical device! \n\tReason: "
                + Tools::Convert::result_to_string(result) + "\n\tDescription: " + Tools::Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return device;
    }

    EVK_MAYBE_UNUSED static bool CopyBufferToImage(Types::CmdBuffer* copyCmd, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        if (!copyCmd->IsBegin())
            copyCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
                width,
                height,
                1
        };

        vkCmdCopyBufferToImage(*copyCmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        copyCmd->End();

        return true;
    }

    EVK_MAYBE_UNUSED static VkSampler CreateSampler(
        const Types::Device* pDevice,
        uint32_t mipLevels,
        VkFilter minFilter,
        VkFilter magFilter,
        VkSamplerAddressMode addressMode,
        VkCompareOp compareOp
    ) {
        VkSamplerCreateInfo samplerIC = Tools::Initializers::SamplerCreateInfo();

        if (minFilter < VK_FILTER_NEAREST || minFilter >= VK_FILTER_MAX_ENUM) {
            VK_HALT("Invalid min filter!");
            return VK_NULL_HANDLE;
        }

        if (magFilter < VK_FILTER_NEAREST || magFilter >= VK_FILTER_MAX_ENUM) {
            VK_HALT("Invalid mag filter!");
            return VK_NULL_HANDLE;
        }

        samplerIC.magFilter    = magFilter;
        samplerIC.minFilter    = minFilter;
        samplerIC.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerIC.addressModeU = addressMode;
        samplerIC.addressModeV = addressMode;
        samplerIC.addressModeW = addressMode;
        samplerIC.mipLodBias   = 0.0f;
        samplerIC.compareOp    = compareOp;
        samplerIC.minLod       = 0.0f;

        /// Set max level-of-detail to mip level count of the texture
        samplerIC.maxLod = mipLevels;

        /// Enable anisotropic filtering
        /// This feature is optional, so we must check if it's supported on the device
        if (pDevice->SamplerAnisotropyEnabled()) {
            /// Use max. level of anisotropy for this example
            samplerIC.maxAnisotropy    = pDevice->GetMaxSamplerAnisotropy();
            samplerIC.anisotropyEnable = VK_TRUE;
        }
        else {
            /// The device does not support anisotropic filtering
            samplerIC.maxAnisotropy = 1.0;
            samplerIC.anisotropyEnable = VK_FALSE;
        }

        samplerIC.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VkSampler sampler = VK_NULL_HANDLE;
        if (vkCreateSampler(*pDevice, &samplerIC, nullptr, &sampler) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateSampler() : failed to create vulkan sampler!");
            return VK_NULL_HANDLE;
        }

        return sampler;
    }

    EVK_MAYBE_UNUSED static std::set<std::string> GetSupportedInstanceExtensions() {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
        std::set<std::string> results;
        for (auto & extension : extensions) {
            results.insert(extension.extensionName);
        }
        return results;
    }

    /*
    static VkImage CreateImage(
            Types::Device* device,
            uint32_t width, uint32_t height,
            uint32_t mipLevels,
            VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            Types::DeviceMemory* imageMemory,
            bool multisampling = true,
            VkImageCreateFlagBits createFlagBits = VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM,
            uint32_t arrayLayers = 1)
    {
        if (width == 0 || height == 0) {
            VK_ERROR("VulkanTools::CreateImage() : width or height equals zero!");
            return VK_NULL_HANDLE;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = mipLevels;
        imageInfo.arrayLayers   = arrayLayers;
        imageInfo.format        = format;
        imageInfo.tiling        = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usage;
        imageInfo.samples       = (mipLevels > 1 || !multisampling) ? VK_SAMPLE_COUNT_1_BIT : device->GetMSAASamples();
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        if (createFlagBits != VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM)
            imageInfo.flags = createFlagBits;

        VkImage image = VK_NULL_HANDLE;
        if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateImage() : failed to create vulkan image!");
            return VK_NULL_HANDLE;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(*device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;

        VkBool32 found;
        allocInfo.memoryTypeIndex = device->GetMemoryType(memRequirements.memoryTypeBits, properties, &found);
        if (!found) { // TODO
            allocInfo.memoryTypeIndex = device->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &found);
            if (!found) {
                VK_ERROR("Tools::CreateImage() : memory is not support this properties!");
                return VK_NULL_HANDLE;
            }
        }

        *imageMemory = device->AllocateMemory(allocInfo);
        if (!imageMemory->Ready()) {
            VK_ERROR("Tools::CreateImage() : failed to allocate device memory!");
            return VK_NULL_HANDLE;
        }

        if (vkBindImageMemory(*device, image, *imageMemory, 0) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateImage() : failed to bind vulkan image memory!");
            return VK_NULL_HANDLE;
        }

        return image;
    }*/

    EVK_MAYBE_UNUSED static uint8_t GetMaxSamplerAnisotropy(VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        return deviceProperties.limits.maxSamplerAnisotropy;
    }

    EVK_MAYBE_UNUSED static VkImageLayout FindDepthFormatLayout(VkImageAspectFlags aspectMask, bool readOnly, bool separateDepthStencilLayouts) {
        if (!separateDepthStencilLayouts) {
            if (aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT || aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                if (readOnly) {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
                else {
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
            }
            VK_HALT("Unknown aspect mask!");
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }

        if (aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT) {
            if (readOnly) {
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            }
            else {
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            }
        }
        else if (aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT) {
            if (readOnly) {
                return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
            }
            else {
                return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }
        else if (aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
            if (readOnly) {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            else {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }

        VK_HALT("Unknown aspect mask!");
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    EVK_MAYBE_UNUSED static bool TransitionImageLayoutEx(
            Types::CmdBuffer* copyCmd,
            VkImage image,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels,
            VkImageAspectFlags aspectMask,
            uint32_t layerCount,
            bool needEnd = true
    ) {
        if (!copyCmd->IsBegin())
            copyCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkImageMemoryBarrier barrier = { };
        barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout            = oldLayout;
        barrier.newLayout            = newLayout;
        barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                = image;
        barrier.subresourceRange.aspectMask     = aspectMask;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = layerCount;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
           VK_ERROR("Tools::TransitionImageLayout() : unsupported layout transition!");
           return false;
        }

        vkCmdPipelineBarrier(
                *copyCmd,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );

        return !needEnd || copyCmd->End();
    }

    EVK_MAYBE_UNUSED static bool TransitionImageLayout(
            Types::CmdBuffer* copyCmd,
            VkImage image,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels,
            uint32_t layerCount,
            bool needEnd = true
    ) {
        return TransitionImageLayoutEx(copyCmd, image, oldLayout, newLayout, mipLevels, VK_IMAGE_ASPECT_COLOR_BIT, layerCount, needEnd);
    }
}

#endif //EVOVULKAN_VULKANTOOLS_H
