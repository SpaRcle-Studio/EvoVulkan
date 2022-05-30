//
// Created by Monika on 20.08.2021.
//

#include <EvoVulkan/Tools/VulkanDebug.h>

bool EvoVulkan::Tools::VkDebug::Ready() const {
    if (LogCallback && WarnCallback && ErrorCallback && GraphCallback && AssertCallback) {
        return true;
    }

    std::cerr << "Evo vulkan debugger isn't initialized!\n";

    return false;
}

void EvoVulkan::Tools::VkDebug::Error(const std::string &msg) {
    if (Ready()) {
        ErrorCallback(msg);
    }
}

void EvoVulkan::Tools::VkDebug::Warn(const std::string &msg) {
    if (Ready()) {
        WarnCallback(msg);
    }
}

void EvoVulkan::Tools::VkDebug::Log(const std::string &msg) {
    if (Ready()) {
        LogCallback(msg);
    }
}

void EvoVulkan::Tools::VkDebug::Graph(const std::string &msg) {
    if (Ready()) {
        GraphCallback(msg);
    }
}

bool EvoVulkan::Tools::VkDebug::Assert(const std::string &msg) {
    if (Ready()) {
        return AssertCallback(msg);
    }

    return true;
}