//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_ALLOCATOR_H
#define EVOVULKAN_ALLOCATOR_H

#include "vk_mem_alloc.h"

#include <EvoVulkan/Tools/NonCopyable.h>
#include <EvoVulkan/Types/Image.h>

namespace EvoVulkan::Types {
    class Device;
}

namespace EvoVulkan::Memory {
    class Allocator;

    struct Buffer {
        VkBuffer m_buffer;
        VmaAllocation m_allocation;
    };

    struct RawMemory {
        friend class Allocator;
    private:
        uint64_t       m_size;
        VkDeviceMemory m_memory;
    public:
        RawMemory() {
            m_size   = 0;
            m_memory = VK_NULL_HANDLE;
        }
    public:
        [[nodiscard]] bool Ready() const { return m_memory != VK_NULL_HANDLE && m_size > 0; }
    public:
        operator VkDeviceMemory() const {
            return m_memory;
        }
    };

    class Allocator : public Tools::NonCopyable {
    private:
        explicit Allocator(Types::Device* device)
            : m_device(device)
        { }

        ~Allocator();

    public:
        static Allocator* Create(Types::Device* device);

        operator VmaAllocator() const { return m_vmaAllocator; }

        void Destroy() { }
        void Free();

    public:
        Buffer AllocBuffer(const VkBufferCreateInfo& info, VmaMemoryUsage usage);
        void FreeBuffer(Buffer& info);

        Types::Image AllocImage(const VkImageCreateInfo& info, bool CPUUsage);
        void FreeImage(Types::Image& image);

        RawMemory AllocateMemory(VkMemoryAllocateInfo memoryAllocateInfo);
        bool FreeMemory(RawMemory* memory);

        [[nodiscard]] uint64_t GetGPUMemoryUsage() const;
        [[nodiscard]] uint64_t GetCPUMemoryUsage() const;
        [[nodiscard]] uint64_t GetAllocatedMemorySize() const { return m_deviceMemoryAllocSize; }
        [[nodiscard]] uint64_t GetAllocatedHeapsCount() const { return m_allocHeapsCount;       }

    private:
        bool Init();

    private:
        Types::Device* m_device       = nullptr;
        VmaAllocator   m_vmaAllocator = VK_NULL_HANDLE;

        uint64_t       m_deviceMemoryAllocSize   = 0;
        uint32_t       m_allocHeapsCount         = 0;

    };

}

#endif //EVOVULKAN_ALLOCATOR_H
