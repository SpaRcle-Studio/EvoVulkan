//
// Created by Monika on 01.02.2022.
//

#define VMA_IMPLEMENTATION
#include <EvoVulkan/Memory/Allocator.h>
#include <EvoVulkan/Tools/VulkanDebug.h>

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

    vmaAllocationCreateInfo.flags = 0;
    vmaAllocationCreateInfo.physicalDevice = *m_device;
    vmaAllocationCreateInfo.device = *m_device;
    vmaAllocationCreateInfo.instance = m_device->GetInstance();
    vmaAllocationCreateInfo.pHeapSizeLimit = nullptr;
    vmaAllocationCreateInfo.pTypeExternalMemoryHandleTypes = nullptr;
    vmaAllocationCreateInfo.pVulkanFunctions = nullptr;

    if (auto result = vmaCreateAllocator(&vmaAllocationCreateInfo, &m_vmaAllocator); result != VK_SUCCESS) {
        VK_ERROR("Allocator::Init() : failed to create vma allocator! "
                 "\n\tReason: " + Tools::Convert::result_to_string(result) +
                 "\n\tDescription: " + Tools::Convert::result_to_description(result)
         );
        return false;
    }

    return true;
}

EvoVulkan::Memory::Allocator::~Allocator() {
    m_device = nullptr;

    if (m_vmaAllocator) {
        vmaDestroyAllocator(m_vmaAllocator);
        m_vmaAllocator = VK_NULL_HANDLE;
    }
}
