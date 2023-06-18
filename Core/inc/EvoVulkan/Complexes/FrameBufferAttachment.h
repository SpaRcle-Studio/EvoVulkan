//
// Created by Monika on 18.06.2023.
//

#ifndef SRENGINE_FRAMEBUFFERATTACHMENT_H
#define SRENGINE_FRAMEBUFFERATTACHMENT_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Image.h>
#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanTools.h>

namespace EvoVulkan::Complexes {
    class DLL_EVK_EXPORT FrameBufferAttachment : public Tools::NonCopyable {
    public:
        FrameBufferAttachment() = default;
        ~FrameBufferAttachment() override;

        FrameBufferAttachment(FrameBufferAttachment&& attachment) noexcept {
            m_image = std::exchange(attachment.m_image, {});
            m_view = std::exchange(attachment.m_view, {});
            m_format = std::exchange(attachment.m_format, {});
            m_device = std::exchange(attachment.m_device, {});
            m_allocator = std::exchange(attachment.m_allocator, {});
        }

        FrameBufferAttachment& operator=(FrameBufferAttachment&& attachment) noexcept {
            m_image = std::exchange(attachment.m_image, {});
            m_view = std::exchange(attachment.m_view, {});
            m_format = std::exchange(attachment.m_format, {});
            m_device = std::exchange(attachment.m_device, {});
            m_allocator = std::exchange(attachment.m_allocator, {});

            return *this;
        }

    public:
        static std::unique_ptr<FrameBufferAttachment> CreateColorAttachment(
            EvoVulkan::Memory::Allocator* allocator,
            EvoVulkan::Types::CmdPool* pool,
            VkFormat format,
            VkImageUsageFlags usage,
            VkExtent2D imageSize,
            uint32_t samplesCount,
            uint32_t layersCount,
            uint32_t layer
        );

        static std::unique_ptr<FrameBufferAttachment> CreateDepthAttachment(
            EvoVulkan::Memory::Allocator* allocator,
            EvoVulkan::Types::CmdPool* pPool,
            FrameBufferAttachment* pImageArray,
            VkFormat format,
            VkImageAspectFlags aspect,
            VkExtent2D imageSize,
            uint32_t samplesCount,
            uint32_t layersCount,
            uint32_t layer
        );

        static std::unique_ptr<FrameBufferAttachment> CreateResolveAttachment(
            EvoVulkan::Memory::Allocator* allocator,
            VkFormat format,
            VkExtent2D imageSize,
            uint32_t samplesCount,
            uint32_t layersCount,
            uint32_t layer
        );

        EVK_NODISCARD bool Ready() const;
        EVK_NODISCARD VkImageView GetView() const noexcept { return m_view; }
        EVK_NODISCARD VkFormat GetFormat() const noexcept { return m_format; }
        EVK_NODISCARD Types::Image& GetImage() noexcept { return m_image; }

    private:
        bool m_weakImage = false;
        Types::Image m_image = Types::Image();
        VkImageView m_view = VK_NULL_HANDLE;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        Types::Device* m_device = nullptr;
        EvoVulkan::Memory::Allocator* m_allocator = nullptr;

    };
}

#endif //SRENGINE_FRAMEBUFFERATTACHMENT_H
