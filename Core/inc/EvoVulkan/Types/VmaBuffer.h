//
// Created by Monika on 09.02.2022.
//

#ifndef EVOVULKAN_VMABUFFER_H
#define EVOVULKAN_VMABUFFER_H

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Types {
    class Device;

    struct DLL_EVK_EXPORT VmaBuffer : Tools::NonCopyable {
    private:
        VmaBuffer(Memory::Allocator* allocator, VkDeviceSize size);
        ~VmaBuffer() override = default;

    public:
        operator VkBuffer() const { return m_buffer.m_buffer; }

    public:
        static VmaBuffer* Create(
                Memory::Allocator* allocator,
                VkBufferUsageFlags bufferUsage,
                VmaMemoryUsage memoryUsage,
                VkDeviceSize size,
                void* data = nullptr);

        static VmaBuffer* Create(
                Memory::Allocator* allocator,
                VkDeviceSize size,
                void *data = nullptr);

    public:
        EVK_NODISCARD const VkBuffer* GetCRef() const { return &m_buffer.m_buffer; }
        EVK_NODISCARD VkDescriptorBufferInfo* GetDescriptorRef() { return &m_descriptor; }

        void CopyToDevice(void *data, bool flush = false);
        void SetupDescriptor(VkDeviceSize offset = 0);

        VkResult Flush();
        VkResult Bind();
        VkResult Map();
        void* MapData();
        void Unmap();

        void Destroy();
        void Free();

    private:
        void*                  m_mapped     = nullptr;
        Memory::Allocator*     m_allocator  = nullptr;
        Memory::Buffer         m_buffer     = { };
        VkDescriptorBufferInfo m_descriptor = { };
        VkDeviceSize           m_size       = 0;

    };
}

#endif //EVOVULKAN_VMABUFFER_H
