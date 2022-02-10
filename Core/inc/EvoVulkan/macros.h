//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_MACROS_H
#define EVOVULKAN_MACROS_H

#ifdef __MINGW32__
    #pragma GCC diagnostic ignored "-Wattributes"
#endif

#define EVK_DEBUG
#define EVK_UNUSED [[maybe_unused]]

#define EVK_INLINE inline

#include <vk_mem_alloc.h>
#include <string>
#include <vector>

#endif //EVOVULKAN_MACROS_H
