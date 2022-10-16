//
// Created by Monika on 07.02.2022.
//

#ifndef EVOVULKAN_IMAGE_H
#define EVOVULKAN_IMAGE_H

#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Memory {
    class Allocator;
}

namespace EvoVulkan::Types {
    struct DLL_EVK_EXPORT ImageCreateInfo {
        ImageCreateInfo() = default;

        ImageCreateInfo(Types::Device* _device,
                        Memory::Allocator* _allocator,
                        uint32_t _width, uint32_t _height,
                        VkImageUsageFlags _usage,
                        uint8_t _sampleCount = 0)
            : ImageCreateInfo(_device, _allocator, _width, _height, VK_FORMAT_UNDEFINED, _usage, _sampleCount)
        { }

        ImageCreateInfo(Types::Device* _device,
                        Memory::Allocator* _allocator,
                        uint32_t _width, uint32_t _height,
                        VkFormat _format,
                        VkImageUsageFlags _usage,
                        uint8_t _sampleCount = 0,
                        bool _cpuUsage = false,
                        uint32_t _mipLevels = 1,
                        uint32_t _arrayLayers = 1,
                        VkImageCreateFlagBits _flags = VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM)
            : device(_device)
            , allocator(_allocator)
            , width(_width)
            , height(_height)
            , usage(_usage)
            , sampleCount(_sampleCount)
            , CPUUsage(_cpuUsage)
            , format(_format)
            , mipLevels(_mipLevels)
            , createFlagBits(_flags)
            , arrayLayers(_arrayLayers)
        {
            tiling = _cpuUsage ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        }

        Types::Device* device = nullptr;
        Memory::Allocator* allocator = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageTiling tiling = VK_IMAGE_TILING_MAX_ENUM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
        uint8_t sampleCount = 0;
        VkImageCreateFlagBits createFlagBits = VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM;
        uint32_t arrayLayers = 1;
        bool CPUUsage = false;

        EVK_NODISCARD bool Valid() const {
            return width > 0 && height > 0 && device && allocator;
        }
    };

    class DLL_EVK_EXPORT Image : public Tools::NonCopyable {
        friend class Memory::Allocator;
    public:
        Image() = default;
        ~Image() override = default;

        Image(Image&& image) noexcept {
            m_image = std::exchange(image.m_image, {});
            m_allocation = std::exchange(image.m_allocation, {});
            m_allocator = std::exchange(image.m_allocator, {});
        }

        Image& operator=(Image&& image) noexcept {
            m_image = std::exchange(image.m_image, {});
            m_allocation = std::exchange(image.m_allocation, {});
            m_allocator = std::exchange(image.m_allocator, {});

            return *this;
        }

    public:
        static Image Create(const ImageCreateInfo& info);

        bool Bind();

        EVK_NODISCARD Image Copy() const;
        EVK_NODISCARD bool Valid() const;

        operator VkImage() const { return m_image; }

    private:
        VkImage m_image            = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocator m_allocator   = VK_NULL_HANDLE;

    };
}

#endif //EVOVULKAN_IMAGE_H
