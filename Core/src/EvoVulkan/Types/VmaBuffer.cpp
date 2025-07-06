//
// Created by Monika on 09.02.2022.
//

#include <EvoVulkan/Memory/Allocator.h>
#include <EvoVulkan/Types/VmaBuffer.h>

namespace EvoVulkan::Types {
    VmaBuffer::~VmaBuffer() {
        m_allocator->FreeBuffer(m_buffer);
    }

    VmaBuffer* VmaBuffer::Create(
            EvoVulkan::Memory::Allocator* allocator,
            VkBufferUsageFlags bufferUsage,
            VmaMemoryUsage memoryUsage,
            VkDeviceSize size,
            const void* data)
    {
        auto buffer = new VmaBuffer(allocator, size);
        auto bufferCreateInfo = Tools::Initializers::BufferCreateInfo(bufferUsage, size);

        buffer->m_buffer = allocator->AllocBuffer(bufferCreateInfo, memoryUsage);

        if (data) {
            buffer->CopyToDevice(data, 0, memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY);
        }

        buffer->SetupDescriptor();

        return buffer;
    }

    VmaBuffer* VmaBuffer::Create(
            Memory::Allocator* allocator,
            VkBufferUsageFlags bufferUsage,
            VmaMemoryUsage memoryUsage,
            VkDeviceSize size,
            VkSharingMode sharingMode,
            VkBufferCreateFlags createFlags,
            VmaAllocationCreateFlags allocateFlags,
            const void* data)
    {
        auto&& buffer = new VmaBuffer(allocator, size);

        auto&& bufferCreateInfo = Tools::Initializers::BufferCreateInfo(bufferUsage, size);
        bufferCreateInfo.sharingMode = sharingMode;
        bufferCreateInfo.flags = createFlags;

        buffer->m_buffer = allocator->AllocBuffer(bufferCreateInfo, memoryUsage, allocateFlags);

        if (data) {
            buffer->CopyToDevice(data, 0, memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY);
        }

        buffer->SetupDescriptor();

        return buffer;
    }

    VmaBuffer* VmaBuffer::Create(EvoVulkan::Memory::Allocator* allocator, VkDeviceSize size, const void* data) {
        return Create(allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size, data);
    }

    EvoVulkan::Types::VmaBuffer::VmaBuffer(EvoVulkan::Memory::Allocator *allocator, VkDeviceSize size)
        : m_allocator(allocator)
        , m_size(size)
    { }

    void EvoVulkan::Types::VmaBuffer::CopyToDevice(const void *data, uint64_t size, bool flush) {
        Map();

        if (size == 0) {
            memcpy(m_mapped, data, m_size);
        }
        else {
            if (size > m_size) {
                VK_ERROR("Buffer::CopyToDevice() : size is greater than buffer size!");
                return;
            }
            memcpy(m_mapped, data, size);
        }

        if (flush) {
            Flush(0, size == 0 ? m_size : size);
        }

        Unmap();
    }

    void VmaBuffer::CopyFromDevice(void* data, uint64_t size) {
        Map();

        if (size == 0) {
            memcpy(data, m_mapped, m_size);
        }
        else {
            if (size > m_size) {
                VK_ERROR("Buffer::CopyFromDevice() : size is greater than buffer size!");
                return;
            }
            memcpy(data, m_mapped, size);
        }

        Unmap();
    }

    VkResult EvoVulkan::Types::VmaBuffer::Map() {
        if (m_buffer.m_allocation == VK_NULL_HANDLE) {
            return VkResult::VK_INCOMPLETE;
        }

        if (m_mapped) {
            VK_ERROR("Buffer::Map() : memory is already mapped!");
            return VkResult::VK_ERROR_MEMORY_MAP_FAILED;
        }

        return vmaMapMemory(*m_allocator, m_buffer.m_allocation, &m_mapped);
    }

    void *EvoVulkan::Types::VmaBuffer::MapData() {
        if (auto result = Map(); result == VK_SUCCESS) {
            return m_mapped;
        }
        else {
            VK_ERROR("Buffer::MapData() : failed to map memory!"
                     "\n\tReason: " + Tools::Convert::result_to_string(result) +
                     "\n\tDescription: " + Tools::Convert::result_to_description(result)
            );

            return nullptr;
        }
    }

    void EvoVulkan::Types::VmaBuffer::Unmap() {
        if (m_mapped) {
            vmaUnmapMemory(*m_allocator, m_buffer.m_allocation);
            m_mapped = nullptr;
        }
    }

    VkResult EvoVulkan::Types::VmaBuffer::Flush() {
        return Flush(0, m_size);
    }

    VkResult EvoVulkan::Types::VmaBuffer::Flush(uint64_t offset, uint64_t size) {
        if (!m_mapped) {
            VK_ERROR("Buffer::Flush() : memory is not mapped!");
            return VkResult::VK_INCOMPLETE;
        }

        if (size + offset > m_size) {
            VK_ERROR("Buffer::Flush() : size is greater than buffer size!");
            return VkResult::VK_INCOMPLETE;
        }

        return vmaFlushAllocation(*m_allocator, m_buffer.m_allocation, offset, size);
    }

    void EvoVulkan::Types::VmaBuffer::SetupDescriptor(VkDeviceSize offset) {
        m_descriptor.offset = offset;
        m_descriptor.buffer = m_buffer.m_buffer;
        m_descriptor.range  = m_size;
    }

    VkResult EvoVulkan::Types::VmaBuffer::Bind() {
        return vmaBindBufferMemory(*m_allocator, m_buffer.m_allocation, m_buffer.m_buffer);
    }
}