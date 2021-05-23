//
// Created by Nikita on 12.04.2021.
//

#include "Example.h"

int main() {
    EvoVulkan::Tools::VkDebug::Error = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "[Error] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Graph = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "[Graph] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Log = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "[Log] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Warn = std::function<void(const std::string& msg)>([](const std::string& msg) {
        std::cout << "[Warn] "  << msg << std::endl;
    });

    //!=================================================================================================================

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    unsigned int width     = 600; //1280
    unsigned int height    = 600; //820
    bool validationEnabled = true;

    auto window = glfwCreateWindow((int)width, (int)height, "Vulkan application", nullptr, nullptr); //1280, 1024
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        auto kernel = static_cast<Core::VulkanKernel*>(glfwGetWindowUserPointer(window));
        kernel->SetSize(width, height);
    });

    //!=================================================================================================================

    auto* kernel = new VulkanExample();
    glfwSetWindowUserPointer(window, (void*)kernel);

    kernel->SetValidationLayersEnabled(validationEnabled);
    kernel->SetSize(width, height);

    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    if (validationEnabled)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (!kernel->PreInit("Simple engine", "NoEngine",
            { extensions },
            { "VK_LAYER_KHRONOS_validation" }))
    {
        std::cout << "Failed to pre-initialize Evo Vulkan!\n";
        return -1;
    }

    std::function<VkSurfaceKHR(const VkInstance& instance)> surfCreate = [window](const VkInstance& instance) -> VkSurfaceKHR {
        VkSurfaceKHR surfaceKhr = {};
        if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceKhr) != VK_SUCCESS) {
            EvoVulkan::Tools::VkDebug::Error("VulkanKernel::Init(lambda) : failed to create glfw window surface!");
            return VK_NULL_HANDLE;
        } else
            return surfaceKhr;
    };

    if (!kernel->Init(
            surfCreate,
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
            true, // sample shading
            true  // vsync
    )) {
        std::cout << "Failed to initialize Evo Vulkan!\n";
        return -1;
    }

    if (!kernel->PostInit()) {
        std::cout << "Failed to post-initialize Evo Vulkan!\n";
        return -1;
    }

    //!=================================================================================================================

    if (!kernel->LoadTexture())
        return -1;

    if (!kernel->SetupShader())
        return -1;

    if (!kernel->SetupUniforms())
        return -1;

    if (!kernel->GenerateGeometry())
        return -1;

    kernel->BuildCmdBuffers();
    //kernel->UpdateUBO();

    while (!glfwWindowShouldClose(window) && !kernel->HasErrors()) {
        glfwPollEvents();

        kernel->NextFrame();

        kernel->UpdateUBO();
    }

    kernel->Destroy();

    delete kernel;

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}