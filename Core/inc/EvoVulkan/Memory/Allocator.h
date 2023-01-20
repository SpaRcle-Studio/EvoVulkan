//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_ALLOCATOR_H
#define EVOVULKAN_ALLOCATOR_H

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>
#include <EvoVulkan/Types/Base/VulkanObject.h>
#include <EvoVulkan/VmaUsage.h>

namespace EvoVulkan::Types {
    class Device;
    class Image;
}

namespace EvoVulkan::Memory {
    class Allocator;

    struct DLL_EVK_EXPORT Buffer {
        VkBuffer m_buffer;
        VmaAllocation m_allocation;
    };

    struct DLL_EVK_EXPORT RawMemory {
        friend class Allocator;
    public:
        RawMemory()
            : m_size(0)
            , m_memory(VK_NULL_HANDLE)
        { }

        operator VkDeviceMemory() const { return m_memory; }

    public:
        EVK_NODISCARD bool Ready() const { return m_memory != VK_NULL_HANDLE && m_size > 0; }

    private:
        uint64_t       m_size;
        VkDeviceMemory m_memory;

    };

    class DLL_EVK_EXPORT Allocator : public Types::IVkObject {
    private:
        explicit Allocator(Types::Device* device)
            : m_device(device)
        { }

    public:
        ~Allocator() override;

        static Allocator* Create(Types::Device* device);

        operator VmaAllocator() const { return m_vmaAllocator; }

    public:
        Buffer AllocBuffer(const VkBufferCreateInfo& info, VmaMemoryUsage usage);
        Types::Image AllocImage(const VkImageCreateInfo& info, bool CPUUsage);
        RawMemory AllocateMemory(VkMemoryAllocateInfo memoryAllocateInfo);

        void FreeBuffer(Buffer& info);
        void FreeImage(Types::Image& image);
        bool FreeMemory(RawMemory* memory);

        EVK_NODISCARD uint64_t GetGPUMemoryUsage() const;
        EVK_NODISCARD uint64_t GetCPUMemoryUsage() const;
        EVK_NODISCARD uint64_t GetAllocatedMemorySize() const { return m_deviceMemoryAllocSize; }
        EVK_NODISCARD uint64_t GetAllocatedHeapsCount() const { return m_allocHeapsCount;       }

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
