//
// Created by Monika on 01.02.2022.
//

#include <EvoVulkan/Memory/Allocator.h>

#define VMA_IMPLEMENTATION

#define VMA_ASSERT(expr) VK_ASSERT(expr)
#include "vk_mem_alloc.h"

EvoVulkan::Memory::Allocator *EvoVulkan::Memory::Allocator::Create(EvoVulkan::Types::Device *device) {
    auto allocator = new Allocator(device);

    if (!allocator->Init()) {
        VK_ERROR("Allocator::Create() : failed to initialize allocator!");
        delete allocator;
        return nullptr;
    }

    return allocator;
}

bool EvoVulkan::Memory::Allocator::Init() {
    VmaAllocatorCreateInfo vmaAllocationCreateInfo;

    auto instance = m_device->GetInstance();

    vmaAllocationCreateInfo.flags = 0;
    vmaAllocationCreateInfo.physicalDevice = *m_device;
    vmaAllocationCreateInfo.device = *m_device;
    vmaAllocationCreateInfo.preferredLargeHeapBlockSize = 256 * 1024 * 1024;
    vmaAllocationCreateInfo.pAllocationCallbacks = nullptr;
    vmaAllocationCreateInfo.pDeviceMemoryCallbacks = nullptr;
    vmaAllocationCreateInfo.instance = *instance;
    vmaAllocationCreateInfo.pHeapSizeLimit = nullptr;
    vmaAllocationCreateInfo.pTypeExternalMemoryHandleTypes = nullptr;
    vmaAllocationCreateInfo.pVulkanFunctions = nullptr;
    vmaAllocationCreateInfo.vulkanApiVersion = instance->GetVersion();

    if (auto result = vmaCreateAllocator(&vmaAllocationCreateInfo, &m_vmaAllocator); result != VK_SUCCESS) {
        VK_ERROR("Allocator::Init() : failed to create vma allocator! "
                 "\n\tReason: " + Tools::Convert::result_to_string(result) +
                 "\n\tDescription: " + Tools::Convert::result_to_description(result)
         );
        return false;
    }

    return true;
}

EvoVulkan::Types::Image EvoVulkan::Memory::Allocator::AllocImage(const VkImageCreateInfo &info, bool CPUUsage) {
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.flags = 0;
    allocCreateInfo.usage = CPUUsage ? VMA_MEMORY_USAGE_CPU_ONLY : VMA_MEMORY_USAGE_GPU_ONLY;
    allocCreateInfo.requiredFlags = 0;
    allocCreateInfo.preferredFlags = 0;
    allocCreateInfo.memoryTypeBits = 0;
    allocCreateInfo.pool = nullptr;
    allocCreateInfo.pUserData = nullptr;

    Types::Image image = {};

    image.m_allocator = m_vmaAllocator;

    auto result = vmaCreateImage(m_vmaAllocator, &info, &allocCreateInfo, &image.m_image, &image.m_allocation, nullptr);

    if (result != VK_SUCCESS) {
        VK_ERROR("Allocator::AllocImage() : failed to create image! "
                 "\n\tReason: " + Tools::Convert::result_to_string(result) +
                 "\n\tDescription: " + Tools::Convert::result_to_description(result)
        );
        return EvoVulkan::Types::Image();
    }

    return image;
}

EvoVulkan::Memory::Allocator::~Allocator() {
    m_device = nullptr;

    if (m_vmaAllocator) {
        vmaDestroyAllocator(m_vmaAllocator);
        m_vmaAllocator = VK_NULL_HANDLE;
    }
}

void EvoVulkan::Memory::Allocator::Free() {
    VK_LOG("Allocator::Free() : free allocator pointer...");
    delete this;
}

void EvoVulkan::Memory::Allocator::FreeImage(Types::Image& image) {
    if (image.m_allocator != m_vmaAllocator) {
        VK_ERROR("Allocator::FreeImage() : allocators is different!");
        return;
    }

    vmaDestroyImage(m_vmaAllocator, image.m_image, image.m_allocation);

    image.m_image      = VK_NULL_HANDLE;
    image.m_allocation = VK_NULL_HANDLE;
    image.m_allocator  = VK_NULL_HANDLE;
}

EvoVulkan::Memory::RawMemory EvoVulkan::Memory::Allocator::AllocateMemory(VkMemoryAllocateInfo memoryAllocateInfo) {
    auto memory = RawMemory();
    memory.m_size = memoryAllocateInfo.allocationSize;
    auto result = vkAllocateMemory(*m_device, &memoryAllocateInfo, nullptr, &memory.m_memory);
    if (result != VK_SUCCESS || memory == VK_NULL_HANDLE) {
        VK_ERROR("Allocator::AllocateMemory : failed to allocate memory! Reason: "
                 + Tools::Convert::result_to_description(result));
        return RawMemory();
    }
    else {
        return memory;
    }
}

bool EvoVulkan::Memory::Allocator::FreeMemory(EvoVulkan::Memory::RawMemory *memory) {
    if (!memory) {
        Tools::VkDebug::Error("Allocator::FreeMemory() : memory is nullptr!");
        return false;
    }

    if (memory->m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(*m_device, *memory, nullptr);
        memory->m_memory = VK_NULL_HANDLE;
        memory->m_size   = 0;
        return true;
    }
    else {
        Tools::VkDebug::Error("Allocator::FreeMemory() : vulkan memory is nullptr!");
        return false;
    }
}

uint64_t EvoVulkan::Memory::Allocator::GetGPUMemoryUsage() const {
    if (m_vmaAllocator) {
        VmaBudget heaps[VK_MAX_MEMORY_HEAPS] = {};
        vmaGetHeapBudgets(m_vmaAllocator, heaps);

        uint64_t totalBytes = 0;

        for (const auto& heap : heaps)
            totalBytes += heap.allocationBytes;

        return totalBytes;
    }

    return 0;
}

uint64_t EvoVulkan::Memory::Allocator::GetCPUMemoryUsage() const {
    return 0;
}

EvoVulkan::Memory::Buffer EvoVulkan::Memory::Allocator::AllocBuffer(const VkBufferCreateInfo &info, VmaMemoryUsage usage) {
    EvoVulkan::Memory::Buffer buffer = {};

    VmaAllocationCreateInfo allocInfo;
    allocInfo.flags = 0;
    allocInfo.usage = usage;
    allocInfo.requiredFlags = 0;
    allocInfo.preferredFlags = 0;
    allocInfo.memoryTypeBits = 0;
    allocInfo.pool = nullptr;
    allocInfo.pUserData = nullptr;

    const auto result = vmaCreateBuffer(m_vmaAllocator, &info, &allocInfo, &buffer.m_buffer, &buffer.m_allocation, nullptr);
    if (result != VK_SUCCESS) {
        VK_ERROR("Allocator::AllocBuffer() : failed to create buffer! "
                 "\n\tReason: " + Tools::Convert::result_to_string(result) +
                 "\n\tDescription: " + Tools::Convert::result_to_description(result)
        );
        return EvoVulkan::Memory::Buffer();
    }

    return buffer;
}

void EvoVulkan::Memory::Allocator::FreeBuffer(EvoVulkan::Memory::Buffer &info) {
    vmaDestroyBuffer(m_vmaAllocator, info.m_buffer, info.m_allocation);

    info.m_buffer = VK_NULL_HANDLE;
    info.m_allocation = VK_NULL_HANDLE;
}
