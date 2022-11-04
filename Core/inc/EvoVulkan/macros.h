//
// Created by Monika on 01.02.2022.
//

#ifndef EVOVULKAN_MACROS_H
#define EVOVULKAN_MACROS_H

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#ifdef __MINGW32__
    #define EVK_MINGW
    #pragma GCC diagnostic ignored "-Wattributes"
#endif

#ifdef __linux__
    #define EVK_LINUX
#endif

#ifdef WIN32
    #define EVK_WIN32
#endif

#define EVK_EXPORTS 1

#ifndef EVK_LINUX
    #if defined(EVK_EXPORTS)
        #define DLL_EVK_EXPORT __declspec(dllexport)
    #else
        #define DLL_EVK_EXPORT __declspec(dllimport)
    #endif
#else
    #define DLL_EVK_EXPORT
#endif


#define EVK_DEBUG
#define EVK_UNUSED [[maybe_unused]]
#define EVK_MAYBE_UNUSED EVK_UNUSED
#define EVK_NODISCARD [[nodiscard]]
#define EVK_FALLTHROUGH [[fallthrough]]

#define EVK_INLINE inline
#define EVK_ID_INVALID -1

#define EVK_MAX(a, b) (a > b ? a : b)
#define EVK_MIN(a, b) (a < b ? a : b)
#define EVK_CLAMP(x, upper, lower) (EVK_MIN(upper, EVK_MAX(x, lower)))

#define VK_PROTOTYPES

#ifdef EVK_WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

#include <direct.h>
#include <variant>
#include <string>
#include <iostream>
#include <functional>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <stack>
#include <mutex>
#include <sys/stat.h>
#include <fstream>
#include <array>
#include <cstring>
#include <cmath>
#include <unordered_set>
#include <unordered_map>

#endif //EVOVULKAN_MACROS_H
