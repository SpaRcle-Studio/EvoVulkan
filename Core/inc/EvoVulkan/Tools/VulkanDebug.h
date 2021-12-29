//
// Created by Nikita on 14.04.2021.
//

#ifndef EVOVULKAN_VULKANDEBUG_H
#define EVOVULKAN_VULKANDEBUG_H

#include <iostream>
#include <string>
#include <functional>
#include <set>
#include <vector>

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
        static std::function<void(const std::string &msg)> Assert;
    };
}

#define VK_LOG(msg)   EvoVulkan::Tools::VkDebug::Log(msg)
#define VK_INFO(msg)  EvoVulkan::Tools::VkDebug::Log(msg) // todo!
#define VK_WARN(msg)  EvoVulkan::Tools::VkDebug::Warn(msg)
#define VK_ERROR(msg) EvoVulkan::Tools::VkDebug::Error(msg)
#define VK_GRAPH(msg) EvoVulkan::Tools::VkDebug::Graph(msg)
#define VK_ASSERT(msg) EvoVulkan::Tools::VkDebug::Assert(msg)
#define VK_ASSERT2(expr, msg) { if (!(expr)) EvoVulkan::Tools::VkDebug::Assert(msg); }

#endif //EVOVULKAN_VULKANDEBUG_H
