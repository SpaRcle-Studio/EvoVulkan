//

// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_NONCOPYABLE_H
#define EVOVULKAN_NONCOPYABLE_H

#include <EvoVulkan/macros.h>

namespace EvoVulkan::Tools {
    class DLL_EVK_EXPORT NonCopyable {
    protected:
        constexpr NonCopyable() = default;
        virtual ~NonCopyable() = default;

        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;

    };
}

#endif //EVOVULKAN_NONCOPYABLE_H
