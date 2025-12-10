//
// Created by Monika on 10.12.2025.
//

#ifndef EVO_VULKAN_PROFILE_H
#define EVO_VULKAN_PROFILE_H

#include <EvoVulkan/macros.h>

#ifdef EVO_VULKAN_TRACY_PROFILER
    #include <Tracy.hpp>

    #define EVK_TRACY_IS_PROFILER_CONNECTED (tracy::GetProfiler().IsConnected())
    #define EVK_TRACY_TEXT_N(name, text) ZoneText(text.c_str(), text.size())
    #define EVK_TRACY_ZONE ZoneScoped /** NOLINT */
    #define EVK_TRACY_ZONE_VALUE(value) ZoneValue(value)
    #define EVK_TRACY_ZONE_TEXT(value) ZoneText(value.c_str(), value.size())
    #define EVK_TRACY_ZONE_TEXT_C(value) ZoneText(value, strlen(value))
    #define EVK_TRACY_ZONE_N(name) ZoneScopedN(name)
    #define EVK_TRACY_ZONE_S(name) ZoneTransientN(TracyConcat(__tracy_source_location, TracyLine), name, true)
    #define EVK_TRACY_PLOT(name, val) TracyPlot(name, val)
    #define EVK_TRACY_ZONE_COLOR(color) ZoneColor(color)

    #define EVK_TRACY_THREAD_NAME(name) tracy::SetThreadName(name)
#else
    #define EVK_TRACY_IS_PROFILER_CONNECTED (false)
    #define EVK_TRACY_TEXT_N(name, text)
    #define EVK_TRACY_ZONE SR_NOOP
    #define EVK_TRACY_ZONE_VALUE(value) SR_NOOP
    #define EVK_TRACY_ZONE_TEXT(value) SR_NOOP
    #define EVK_TRACY_ZONE_TEXT_C(value) SR_NOOP
    #define EVK_TRACY_ZONE_N(name) SR_NOOP
    #define EVK_TRACY_ZONE_S(name) SR_NOOP
    #define EVK_TRACY_PLOT(name, val) SR_NOOP
    #define EVK_TRACY_ZONE_COLOR(color)

    #define EVK_TRACY_THREAD_NAME(name)
#endif

#endif //EVO_VULKAN_PROFILE_H
