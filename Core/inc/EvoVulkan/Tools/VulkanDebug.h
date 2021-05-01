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
        static inline std::function<void(const std::string &msg)> Error = std::function<void(const std::string &msg)>();
        static inline std::function<void(const std::string &msg)> Log   = std::function<void(const std::string &msg)>();
        static inline std::function<void(const std::string &msg)> Graph = std::function<void(const std::string &msg)>();
        static inline std::function<void(const std::string &msg)> Warn  = std::function<void(const std::string &msg)>();
    };
}

#endif //EVOVULKAN_VULKANDEBUG_H
