//
// Created by Monika on 20.08.2021.
//

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Tools/FileSystem.h>

namespace EvoVulkan::Tools {
    bool VkFunctionsHolder::Ready() const {
        std::lock_guard lock(m_mutex);

        const bool debugFunctions = LogCallback && WarnCallback && ErrorCallback && GraphCallback && AssertCallback;
        const bool fileSysFunctions = Delete && IsExists && Copy && CreateFolder;
        const bool hashFunctions = GetFileHash && ReadHash && WriteHash;

        if (debugFunctions && fileSysFunctions && hashFunctions) {
            return true;
        }

        std::cerr << "Evo vulkan functions holder isn't initialized!\n"
            << "\tDebug functions: " << (debugFunctions ? "OK" : "FAIL") << "\n"
            << "\tFile-system functions: " << (fileSysFunctions ? "OK" : "FAIL") << "\n"
            << "\tHash functions: " << (hashFunctions ? "OK" : "FAIL") << "\n";

        return false;
    }

    void VkFunctionsHolder::Error(const std::string &msg) {
        std::lock_guard lock(m_mutex);

        if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Errors))) {
            ErrorCallback(msg);
        }
    }

    void VkFunctionsHolder::Warn(const std::string &msg) {
        std::lock_guard lock(m_mutex);

        if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Warns))) {
            WarnCallback(msg);
        }
    }

    void VkFunctionsHolder::Log(const std::string &msg) {
        std::lock_guard lock(m_mutex);

        if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Logs))) {
            LogCallback(msg);
        }
    }

    void VkFunctionsHolder::Graph(const std::string &msg) {
        std::lock_guard lock(m_mutex);

        if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Graphical))) {
            GraphCallback(msg);
        }
    }

    bool VkFunctionsHolder::Assert(const std::string &msg) {
        std::lock_guard lock(m_mutex);

        if (Ready() && (GetLogLevel() & static_cast<int>(LogLevel::Asserts))) {
            return AssertCallback(msg);
        }

        return true;
    }

    void VkFunctionsHolder::PushLogLevel(EvoVulkan::Tools::LogLevel logLevel) {
        std::lock_guard lock(m_mutex);

        if (m_logLevelStack.size() > 1024) {
            VK_HALT("VkFunctionsHolder::PushLogLevel() : buffer overflow!");
            return;
        }

        m_logLevelStack.push(logLevel);
    }

    void VkFunctionsHolder::PopLogLevel() {
        std::lock_guard lock(m_mutex);

        if (m_logLevelStack.empty()) {
            return;
        }

        m_logLevelStack.pop();
    }

    LogLevelFlags VkFunctionsHolder::GetLogLevel() const {
        std::lock_guard lock(m_mutex);

        if (m_logLevelStack.empty()) {
            return static_cast<LogLevelFlags>(LogLevel::Full);
        }

        return (LogLevelFlags) m_logLevelStack.top();
    }

    void VkFunctionsHolder::Reset() {
        std::lock_guard lock(m_mutex);

        ErrorCallback = nullptr;
        LogCallback = nullptr;
        GraphCallback = nullptr;
        WarnCallback = nullptr;
        AssertCallback = nullptr;
        Delete = nullptr;
        IsExists = nullptr;
        Copy = nullptr;
        CreateFolder = nullptr;
        GetFileHash = nullptr;
        ReadHash = nullptr;
        WriteHash = nullptr;
    }
}