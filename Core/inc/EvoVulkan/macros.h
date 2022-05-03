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

#ifdef WIN32
    #include <vulkan/vulkan_win32.h>
#endif

#include <string>
#include <iostream>
#include <functional>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>

#endif //EVOVULKAN_MACROS_H
