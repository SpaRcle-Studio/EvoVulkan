//
// Created by Monika on 09.02.2022.
//

#include <EvoVulkan/Types/VmaBuffer.h>
#include <EvoVulkan/Memory/Allocator.h>

EvoVulkan::Types::VmaBuffer* EvoVulkan::Types::VmaBuffer::Create(
        EvoVulkan::Memory::Allocator* allocator,
        VkBufferUsageFlags bufferUsage,
        VmaMemoryUsage memoryUsage,
        VkDeviceSize size,
        void* data)
{
    auto buffer = new VmaBuffer(allocator, size);
    auto bufferCreateInfo = Tools::Initializers::BufferCreateInfo(bufferUsage, size);

    buffer->m_buffer = allocator->AllocBuffer(bufferCreateInfo, memoryUsage);

    if (data)
        buffer->CopyToDevice(data, memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY);

    buffer->SetupDescriptor();

    return buffer;
}

EvoVulkan::Types::VmaBuffer* EvoVulkan::Types::VmaBuffer::Create(EvoVulkan::Memory::Allocator* allocator, VkDeviceSize size, void* data) {
    return Create(allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size, data);
}

void EvoVulkan::Types::VmaBuffer::Destroy() {
    m_allocator->FreeBuffer(m_buffer);
}

void EvoVulkan::Types::VmaBuffer::Free() {
    delete this;
}

EvoVulkan::Types::VmaBuffer::VmaBuffer(EvoVulkan::Memory::Allocator *allocator, VkDeviceSize size)
    : m_allocator(allocator)
    , m_size(size)
{ }

void EvoVulkan::Types::VmaBuffer::CopyToDevice(void *data, bool flush) {
    Map();
    memcpy(m_mapped, data, m_size);
    if (flush)
        Flush();
    Unmap();
}

VkResult EvoVulkan::Types::VmaBuffer::Map() {
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
    return vmaFlushAllocation(*m_allocator, m_buffer.m_allocation, 0, m_size);
}

void EvoVulkan::Types::VmaBuffer::SetupDescriptor(VkDeviceSize offset) {
    m_descriptor.offset = offset;
    m_descriptor.buffer = m_buffer.m_buffer;
    m_descriptor.range  = m_size;
}

VkResult EvoVulkan::Types::VmaBuffer::Bind() {
    return vmaBindBufferMemory(*m_allocator, m_buffer.m_allocation, m_buffer.m_buffer);
}






