//
// Created by Nikita on 14.04.2021.
//

#ifndef EVOVULKAN_VULKANDEBUG_H
#define EVOVULKAN_VULKANDEBUG_H

#include <EvoVulkan/Tools/Singleton.h>
#include <EvoVulkan/Tools/StringUtils.h>

namespace EvoVulkan::Tools {
    enum class LogLevel : int {
        None = 1 << 0,
        Errors = 1 << 1,
        Logs = 1 << 2,
        Warns = 1 << 3,
        Graphical = 1 << 4,
        Asserts = 1 << 5,

        ErrorsOnly = Errors | Asserts,

        Full = Errors | Logs | Warns | Graphical | Asserts
    };
    typedef int LogLevelFlags;

    class DLL_EVK_EXPORT VkFunctionsHolder : public Tools::Singleton<VkFunctionsHolder> {
        friend class Tools::Singleton<VkFunctionsHolder>;
    public:
        EVK_NODISCARD bool Ready() const;

    public:
        void Error(const std::string &msg);
        void Log(const std::string &msg);
        void Warn(const std::string &msg);
        void Graph(const std::string &msg);
        bool Assert(const std::string &msg);

    public:
        void PushLogLevel(LogLevel logLevel);
        void PopLogLevel();

        EVK_NODISCARD LogLevelFlags GetLogLevel() const;

    public:
        std::function<bool(const std::string& path)> CreateFolder;
        std::function<bool(const std::string& from, const std::string& to)> Copy;
        std::function<bool(const std::string& path)> IsExists;
        std::function<bool(const std::string& path)> Delete;

        std::function<void(const std::string &msg)> ErrorCallback;
        std::function<void(const std::string &msg)> LogCallback;
        std::function<void(const std::string &msg)> GraphCallback;
        std::function<void(const std::string &msg)> WarnCallback;
        std::function<bool(const std::string &msg)> AssertCallback;

    private:
        std::stack<LogLevel> m_logLevelStack;

    };

    static inline void CreatePath(std::string path, uint32_t offset = 0) {
        if (path.empty())
            return;

        if (path.back() != '/')
            path.append("/");

        auto pos = path.find('/', offset);
        if (pos != std::string::npos) {
            auto dir = Tools::Read(path, pos);
            Tools::VkFunctionsHolder::Instance().CreateFolder(dir);
            CreatePath(path, pos + 1);
        }
    }
}

#define EVK_MAKE_ASSERT(msg) std::string(msg).append("\nFile: ")           \
            .append(__FILE__).append("\nLine: ").append(std::to_string(__LINE__)) \

#define VK_LOG(msg)   EvoVulkan::Tools::VkFunctionsHolder::Instance().Log(msg)
#define VK_INFO(msg)  EvoVulkan::Tools::VkFunctionsHolder::Instance().Log(msg) // todo!
#define VK_WARN(msg)  EvoVulkan::Tools::VkFunctionsHolder::Instance().Warn(msg)
#define VK_ERROR(msg) EvoVulkan::Tools::VkFunctionsHolder::Instance().Error(msg)
#define VK_GRAPH(msg) EvoVulkan::Tools::VkFunctionsHolder::Instance().Graph(msg);
#define VK_ASSERT2(expr, msg) (!!(expr) || EvoVulkan::Tools::VkFunctionsHolder::Instance().Assert(EVK_MAKE_ASSERT(msg)))
#define VK_ASSERT(expr) VK_ASSERT2(expr, "An exception has been occured.")
#define VK_HALT(msg) VK_ASSERT2(false, msg)

#define EVK_PUSH_LOG_LEVEL(level) EvoVulkan::Tools::VkFunctionsHolder::Instance().PushLogLevel(level)
#define EVK_POP_LOG_LEVEL() EvoVulkan::Tools::VkFunctionsHolder::Instance().PopLogLevel()

#define EVK_DELETE_FILE(path) EvoVulkan::Tools::VkFunctionsHolder::Instance().Delete(path)
#define EVK_IS_EXISTS(path) EvoVulkan::Tools::VkFunctionsHolder::Instance().IsExists(path)
#define EVK_CREATE_FOLDER(path) EvoVulkan::Tools::VkFunctionsHolder::Instance().CreateFolder(path)
#define EVK_COPY_FILE(from, to) EvoVulkan::Tools::VkFunctionsHolder::Instance().Copy(from, to)

#endif //EVOVULKAN_VULKANDEBUG_H
