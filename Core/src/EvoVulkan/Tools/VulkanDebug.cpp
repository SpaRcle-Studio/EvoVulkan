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
    if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Errors))) {
        ErrorCallback(msg);
    }
}

void EvoVulkan::Tools::VkDebug::Warn(const std::string &msg) {
    if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Warns))) {
        WarnCallback(msg);
    }
}

void EvoVulkan::Tools::VkDebug::Log(const std::string &msg) {
    if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Logs))) {
        LogCallback(msg);
    }
}

void EvoVulkan::Tools::VkDebug::Graph(const std::string &msg) {
    if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Graphical))) {
        GraphCallback(msg);
    }
}

bool EvoVulkan::Tools::VkDebug::Assert(const std::string &msg) {
    if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Asserts))) {
        return AssertCallback(msg);
    }

    return true;
}

void EvoVulkan::Tools::VkDebug::PushLogLevel(EvoVulkan::Tools::LogLevel logLevel) {
    m_logLevelStack.push(logLevel);
}

void EvoVulkan::Tools::VkDebug::PopLogLevel() {
    if (m_logLevelStack.empty()) {
        return;
    }

    m_logLevelStack.pop();
}

EvoVulkan::Tools::LogLevelFlags EvoVulkan::Tools::VkDebug::GetLogLevel() const {
    if (m_logLevelStack.empty()) {
        return static_cast<LogLevelFlags>(LogLevel::Full);
    }

    return (LogLevelFlags)m_logLevelStack.top();
}
