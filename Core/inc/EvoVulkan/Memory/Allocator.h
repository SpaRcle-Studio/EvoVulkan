//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_ALLOCATOR_H
#define EVOVULKAN_ALLOCATOR_H

#include "vk_mem_alloc.h"

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Types {
    class Device;
}

namespace EvoVulkan::Memory {
    class Allocator : public Tools::NonCopyable {
    private:
        explicit Allocator(Types::Device* device)
            : m_device(device)
        { }

        ~Allocator();

    public:
        static Allocator* Create(Types::Device* device);

    public:

    private:
        bool Init();

    private:
        Types::Device* m_device       = nullptr;
        VmaAllocator   m_vmaAllocator = VK_NULL_HANDLE;

    };
}

#endif //EVOVULKAN_ALLOCATOR_H
