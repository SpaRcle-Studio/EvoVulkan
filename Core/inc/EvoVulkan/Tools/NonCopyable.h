//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_NONCOPYABLE_H
#define EVOVULKAN_NONCOPYABLE_H

namespace EvoVulkan::Tools {
    class NonCopyable {
    protected:
        constexpr NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
}

#endif //EVOVULKAN_NONCOPYABLE_H
