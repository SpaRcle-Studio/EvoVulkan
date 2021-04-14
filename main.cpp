//
// Created by Nikita on 12.04.2021.
//

#include <EvoVulkan/VulkanKernel.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main() {
    EvoVulkan::Tools::VkDebug::Error = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "\t[Error] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Graph = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "[Graph] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Log = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "[Log] "  << msg << std::endl;
    });

    //!=================================================================================================================

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(1280, 820, "Vulkan application", nullptr, nullptr); //1280, 1024

    //!=================================================================================================================

    EvoVulkan::Core::VulkanKernel* kernel = EvoVulkan::Core::VulkanKernel::Create();
    kernel->SetValidationLayersEnabled(true);

    if (!kernel->PreInit("Simple engine", "NoEngine", { "VK_LAYER_KHRONOS_validation" })) {
        std::cout << "Failed to pre-initialize Evo Vulkan!\n";
        return -1;
    }

    std::function<VkSurfaceKHR(const VkInstance& instance)> fun = [window](const VkInstance& instance) -> VkSurfaceKHR {
        VkSurfaceKHR surfaceKhr = {};
        if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceKhr) != VK_SUCCESS) {
            EvoVulkan::Tools::VkDebug::Error("VulkanKernel::Init(lambda) : failed to create glfw window surface!");
            return VK_NULL_HANDLE;
        } else
            return surfaceKhr;
    };

    if (!kernel->Init(fun, { VK_KHR_SWAPCHAIN_EXTENSION_NAME })) {
        std::cout << "Failed to initialize Evo Vulkan!\n";
        return -1;
    }

    //!=================================================================================================================

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    kernel->Free();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}