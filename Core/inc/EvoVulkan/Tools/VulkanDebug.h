//
// Created by Nikita on 14.04.2021.
//

#ifndef EVOVULKAN_VULKANDEBUG_H
#define EVOVULKAN_VULKANDEBUG_H

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Tools {
    class VkDebug {
    public:
        VkDebug()               = delete;
        VkDebug(const VkDebug&) = delete;
        ~VkDebug()              = delete;
    public:
        static std::function<void(const std::string &msg)> Error;
        static std::function<void(const std::string &msg)> Log;
        static std::function<void(const std::string &msg)> Graph;
        static std::function<void(const std::string &msg)> Warn;
        static std::function<bool(const std::string &msg)> Assert;
    };
}

#define EVK_MAKE_ASSERT(msg) std::string(msg).append("\nFile: ")           \
            .append(__FILE__).append("\nLine: ").append(std::to_string(__LINE__)) \

#define VK_LOG(msg)   EvoVulkan::Tools::VkDebug::Log(msg)
#define VK_INFO(msg)  EvoVulkan::Tools::VkDebug::Log(msg) // todo!
#define VK_WARN(msg)  EvoVulkan::Tools::VkDebug::Warn(msg)
#define VK_ERROR(msg) EvoVulkan::Tools::VkDebug::Error(msg)
#define VK_GRAPH(msg) EvoVulkan::Tools::VkDebug::Graph(msg)
#define VK_ASSERT2(expr, msg) (!!(expr) || EvoVulkan::Tools::VkDebug::Assert(EVK_MAKE_ASSERT(msg)))
#define VK_ASSERT(expr) VK_ASSERT2(expr, "An exception has been occured.")

#endif //EVOVULKAN_VULKANDEBUG_H
