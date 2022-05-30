//
// Created by Nikita on 14.04.2021.
//

#ifndef EVOVULKAN_VULKANDEBUG_H
#define EVOVULKAN_VULKANDEBUG_H

#include <EvoVulkan/Tools/Singleton.h>

namespace EvoVulkan::Tools {
    class DLL_EVK_EXPORT VkDebug : public Tools::Singleton<VkDebug> {
        friend class Tools::Singleton<VkDebug>;
    protected:
        ~VkDebug() override = default;

    public:
        EVK_NODISCARD bool Ready() const;

    public:
        void Error(const std::string &msg);
        void Log(const std::string &msg);
        void Warn(const std::string &msg);
        void Graph(const std::string &msg);
        bool Assert(const std::string &msg);

    public:
        std::function<void(const std::string &msg)> ErrorCallback;
        std::function<void(const std::string &msg)> LogCallback;
        std::function<void(const std::string &msg)> GraphCallback;
        std::function<void(const std::string &msg)> WarnCallback;
        std::function<bool(const std::string &msg)> AssertCallback;

    };
}

#define EVK_MAKE_ASSERT(msg) std::string(msg).append("\nFile: ")           \
            .append(__FILE__).append("\nLine: ").append(std::to_string(__LINE__)) \

#define VK_LOG(msg)   EvoVulkan::Tools::VkDebug::Instance().Log(msg)
#define VK_INFO(msg)  EvoVulkan::Tools::VkDebug::Instance().Log(msg) // todo!
#define VK_WARN(msg)  EvoVulkan::Tools::VkDebug::Instance().Warn(msg)
#define VK_ERROR(msg) EvoVulkan::Tools::VkDebug::Instance().Error(msg)
#define VK_GRAPH(msg) EvoVulkan::Tools::VkDebug::Instance().Graph(msg);
#define VK_ASSERT2(expr, msg) (!!(expr) || EvoVulkan::Tools::VkDebug::Instance().Assert(EVK_MAKE_ASSERT(msg)))
#define VK_ASSERT(expr) VK_ASSERT2(expr, "An exception has been occured.")

#endif //EVOVULKAN_VULKANDEBUG_H
