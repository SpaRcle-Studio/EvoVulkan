//
// Created by Monika on 07.05.2021.
//

#ifndef EVOVULKAN_VULKANBUFFER_H
#define EVOVULKAN_VULKANBUFFER_H

#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Types {
    class Device;
    /**
    * @brief Encapsulates access to a Vulkan buffer backed up by device memory
    * @note To be filled by an external source like the VulkanDevice
    */
    struct DLL_EVK_EXPORT Buffer : Tools::NonCopyable {
    private:
        Buffer() = default;

    public:
        ~Buffer() override;

        static Buffer* Create(
                Device* device,
                Memory::Allocator* allocator,
                VkBufferUsageFlags usageFlags,
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkDeviceSize size, void *data = nullptr);

        static Buffer* Create(Device* device, Memory::Allocator* allocator, VkDeviceSize size, void *data = nullptr);

    public:
        operator VkBuffer() const { return m_buffer; }

    public:
        VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult Bind(VkDeviceSize offset = 0) const;
        VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        EVK_NODISCARD const VkBuffer* GetCRef() const { return &m_buffer; }
        EVK_NODISCARD VkDescriptorBufferInfo* GetDescriptorRef() { return &m_descriptor; }

        void* MapData();
        void Unmap();

        void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void CopyTo(void *data, VkDeviceSize size);
        void CopyToDevice(void *data, VkDeviceSize size) const;

    private:
        Types::Device*         m_device              = nullptr;
        Memory::Allocator*     m_allocator           = nullptr;
        VkBuffer               m_buffer              = VK_NULL_HANDLE;
        Memory::RawMemory      m_memory              = {};
        VkDescriptorBufferInfo m_descriptor          = {};
        VkDeviceSize           m_size                = 0;
        VkDeviceSize           m_alignment           = 0;
        void*                  m_mapped              = nullptr;
        /** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
        VkBufferUsageFlags     m_usageFlags          = {};
        /** @brief Memory property flags to be filled by external source at buffer creation (to query at some later point) */
        VkMemoryPropertyFlags  m_memoryPropertyFlags = {};

    };
}

#endif //EVOVULKAN_VULKANBUFFER_H
