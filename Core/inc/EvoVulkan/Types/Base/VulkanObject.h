//
// Created by Nikita on 02.05.2021.
//

#ifndef EVOVULKAN_VULKANOBJECT_H
#define EVOVULKAN_VULKANOBJECT_H

#include <EvoVulkan/Tools/NonCopyable.h>

#ifdef EVK_MINGW
    #pragma GCC diagnostic ignored "-Wattributes"
#endif

namespace EvoVulkan::Types {
    class DLL_EVK_EXPORT IVkObject : public Tools::NonCopyable {
    public:
        ~IVkObject() override = default;

    public:
        EVK_NODISCARD virtual bool IsReady()    const { return false; };
        EVK_NODISCARD virtual bool IsComplete() const { return false; };

    };
}

#endif //EVOVULKAN_VULKANOBJECT_H
