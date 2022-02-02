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
    struct Buffer : Tools::NonCopyable {
    private:
        Buffer()  = default;
        ~Buffer() = default;
    public:
        static Buffer* Create(
                Device* device,
                VkBufferUsageFlags usageFlags,
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkDeviceSize size, void *data = nullptr);

        static Buffer* Create(Device* device, VkDeviceSize size, void *data = nullptr);

    public:
        operator VkBuffer() const { return m_buffer; }

    public:
        VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult Bind(VkDeviceSize offset = 0) const;
        VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        [[nodiscard]] const VkBuffer* GetCRef() const { return &m_buffer; }
        [[nodiscard]] VkDescriptorBufferInfo* GetDescriptorRef() { return &m_descriptor; }

        void* MapData();
        void Unmap();

        void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void CopyTo(void *data, VkDeviceSize size);
        void CopyToDevice(void *data, VkDeviceSize size) const;

        void Destroy();
        void Free();

    private:
        Types::Device*         m_device              = nullptr;
        VkBuffer               m_buffer              = VK_NULL_HANDLE;
        Types::DeviceMemory    m_memory              = {};
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
