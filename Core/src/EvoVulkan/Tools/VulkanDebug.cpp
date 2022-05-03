//
// Created by Monika on 20.08.2021.
//

#include <EvoVulkan/Tools/VulkanDebug.h>

std::function<void(const std::string &msg)> EvoVulkan::Tools::VkDebug::Error  = std::function<void(const std::string &msg)>();
std::function<void(const std::string &msg)> EvoVulkan::Tools::VkDebug::Warn   = std::function<void(const std::string &msg)>();
std::function<void(const std::string &msg)> EvoVulkan::Tools::VkDebug::Log    = std::function<void(const std::string &msg)>();
std::function<void(const std::string &msg)> EvoVulkan::Tools::VkDebug::Graph  = std::function<void(const std::string &msg)>();
std::function<bool(const std::string &msg)> EvoVulkan::Tools::VkDebug::Assert = std::function<bool(const std::string &msg)>();