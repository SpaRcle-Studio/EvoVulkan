//
// Created by Nikita on 07.05.2021.
//

#include "EvoVulkan/Types/VulkanBuffer.h"
#include <cassert>

#include <EvoVulkan/Tools/VulkanInitializers.h>

#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Types {
    /**
    * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
    *
    * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
    * @param offset (Optional) Byte offset from beginning
    *
    * @return VkResult of the buffer mapping call
    */
    VkResult Buffer::Map(VkDeviceSize size, VkDeviceSize offset) {
        return vkMapMemory(*m_device, m_memory, offset, size, 0, &m_mapped);
    }

    /**
    * Unmap a mapped memory range
    *
    * @note Does not return a result as vkUnmapMemory can't fail
    */
    void Buffer::Unmap() {
        if (m_mapped) {
            vkUnmapMemory(*m_device, m_memory);
            m_mapped = nullptr;
        }
    }

    /**
    * Attach the allocated memory block to the buffer
    *
    * @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
    *
    * @return VkResult of the bindBufferMemory call
    */
    VkResult Buffer::Bind(VkDeviceSize offset) const {
        return vkBindBufferMemory(*m_device, m_buffer, m_memory, offset);
    }

    /**
    * Setup the default descriptor for this buffer
    *
    * @param size (Optional) Size of the memory range of the descriptor
    * @param offset (Optional) Byte offset from beginning
    *
    */
    void Buffer::SetupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
        m_descriptor.offset = offset;
        m_descriptor.buffer = m_buffer;
        m_descriptor.range = size;
    }

    /**
    * Copies the specified data to the mapped buffer
    *
    * @param data Pointer to the data to copy
    * @param size Size of the data to copy in machine units
    *
    */
    void Buffer::CopyTo(void *data, VkDeviceSize size) {
        assert(m_mapped);
        memcpy(m_mapped, data, size);
    }

    void Buffer::CopyToDevice(void *data, VkDeviceSize size) const {
        vkMapMemory(*m_device, m_memory, 0, size, 0, (void **)&m_mapped);
        memcpy(m_mapped, data, size);
        // Unmap after data has been copied
        // Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
        vkUnmapMemory(*m_device, m_memory);
    }

    /**
    * Flush a memory range of the buffer to make it visible to the device
    *
    * @note Only required for non-coherent memory
    *
    * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
    * @param offset (Optional) Byte offset from beginning
    *
    * @return VkResult of the flush call
    */
    VkResult Buffer::Flush(VkDeviceSize size, VkDeviceSize offset) const {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(*m_device, 1, &mappedRange);
    }

    /**
    * Invalidate a memory range of the buffer to make it visible to the host
    *
    * @note Only required for non-coherent memory
    *
    * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
    * @param offset (Optional) Byte offset from beginning
    *
    * @return VkResult of the invalidate call
    */
    VkResult Buffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) const {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(*m_device, 1, &mappedRange);
    }

    /**
    * Release all Vulkan resources held by this buffer
    */
    void Buffer::Destroy() {
        if (m_buffer)
            vkDestroyBuffer(*m_device, m_buffer, nullptr);

        m_buffer = VK_NULL_HANDLE;

        if (m_memory.Ready())
            m_device->FreeMemory(&m_memory);
    }

    Buffer* Buffer::Create(
            Device *device,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize size, void* data) {
        auto* buffer = new Buffer();
        buffer->m_device = device;

        // Create the buffer handle
        VkBufferCreateInfo bufferCreateInfo = Tools::Initializers::BufferCreateInfo(usageFlags, size);
        auto result = vkCreateBuffer(*device, &bufferCreateInfo, nullptr, &buffer->m_buffer);
        if (result != VK_SUCCESS) {
            VK_ERROR("Buffer::Create() : failed to create vulkan buffer!");
            return {};
        }

        // Create the memory backing up the buffer handle
        VkMemoryRequirements memReqs;
        VkMemoryAllocateInfo memAlloc = Tools::Initializers::MemoryAllocateInfo();
        vkGetBufferMemoryRequirements(*device, buffer->m_buffer, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        // Find a memory type index that fits the properties of the buffer
        memAlloc.memoryTypeIndex = device->GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            memAlloc.pNext = &allocFlagsInfo;
        }

        if (buffer->m_memory = device->AllocateMemory(memAlloc); !buffer->m_memory.Ready()) {
            VK_ERROR("Buffer::Create() : failed to allocate vulkan memory!");
            return { };
        }

        buffer->m_alignment = memReqs.alignment;
        buffer->m_size = size;
        buffer->m_usageFlags = usageFlags;
        buffer->m_memoryPropertyFlags = memoryPropertyFlags;

        // If a pointer to the buffer data has been passed, map the buffer and copy over the data
        if (data != nullptr) {
            result = buffer->Map();
            if (result != VK_SUCCESS) {
                VK_ERROR("Buffer::Create() : failed to map buffer!");
                return { };
            }

            memcpy(buffer->m_mapped, data, size);
            if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
                buffer->Flush();

            buffer->Unmap();
        }

        // Initialize a default descriptor that covers the whole buffer size
        buffer->SetupDescriptor(buffer->m_size);

        if (buffer->Bind() != VK_SUCCESS) {
            VK_ERROR("Buffer::Create() : failed to bind buffer!");
            return {};
        }

        // Attach the memory to the buffer object
        return buffer;
    }
}