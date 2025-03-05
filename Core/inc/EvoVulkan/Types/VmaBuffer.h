//
// Created by Monika on 09.02.2022.
//

#ifndef EVOVULKAN_VMABUFFER_H
#define EVOVULKAN_VMABUFFER_H

#include <EvoVulkan/Tools/NonCopyable.h>
#include <EvoVulkan/Memory/Allocator.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Types {
    class Device;

    struct VmaBufferDebugInfo {
        uint32_t itemSize = 0;
        uint32_t itemCount = 0;
    };

    class DLL_EVK_EXPORT VmaBuffer : Tools::NonCopyable {
    private:
        VmaBuffer(Memory::Allocator* allocator, VkDeviceSize size);

    public:
        ~VmaBuffer() override;

        operator VkBuffer() const { return m_buffer.m_buffer; }

    public:
        static VmaBuffer* Create(
                Memory::Allocator* allocator,
                VkBufferUsageFlags bufferUsage,
                VmaMemoryUsage memoryUsage,
                VkDeviceSize size,
                const void* data = nullptr);

        static VmaBuffer* Create(
                Memory::Allocator* allocator,
                VkBufferUsageFlags bufferUsage,
                VmaMemoryUsage memoryUsage,
                VkDeviceSize size,
                VkSharingMode sharingMode,
                VkBufferCreateFlags createFlags,
                VmaAllocationCreateFlags allocateFlags,
                const void* data = nullptr);

        static VmaBuffer* Create(
                Memory::Allocator* allocator,
                VkDeviceSize size,
                const void *data = nullptr);

    public:
        EVK_NODISCARD const VmaBufferDebugInfo& GetDebugInfo() const { return m_debugInfo; }
        EVK_NODISCARD const VkBuffer* GetCRef() const { return &m_buffer.m_buffer; }
        EVK_NODISCARD VkDescriptorBufferInfo* GetDescriptorRef() { return &m_descriptor; }

        void SetDebugInfo(const VmaBufferDebugInfo& debugInfo) { m_debugInfo = debugInfo; }
        void CopyToDevice(const void *data, bool flush = false);
        void SetupDescriptor(VkDeviceSize offset = 0);

        VkResult Flush();
        VkResult Bind();
        VkResult Map();
        void* MapData();
        void Unmap();

    private:
        void*                  m_mapped     = nullptr;
        Memory::Allocator*     m_allocator  = nullptr;
        Memory::Buffer         m_buffer     = { };
        VkDescriptorBufferInfo m_descriptor = { };
        VkDeviceSize           m_size       = 0;
        VmaBufferDebugInfo     m_debugInfo  = { };

    };
}

#endif //EVOVULKAN_VMABUFFER_H
