//
// Created by Monika on 26.05.2022.
//

#include <EvoVulkan/Tools/Singleton.h>

namespace EvoVulkan::Tools {
    template<typename T> T& Singleton<T>::Instance() {
        auto&& singleton = GetSingleton();

        if (!(*singleton)) {
            *singleton = new Singleton<T>();
        }

        return (*singleton)->InternalInstance();
    }

    template<typename T> void Singleton<T>::Destroy() {
        auto&& singleton = GetSingleton();

        if (!(*singleton)) {
            return;
        }

        (*singleton)->InternalDestroy();

        delete *singleton;
        (*singleton) = nullptr;
    }

    template<typename T> Singleton <T> **Singleton<T>::GetSingleton() {
        static Singleton<T>* singleton = nullptr;
        return &singleton;
    }

    template<typename T> T &Singleton<T>::InternalInstance() {
        if (!m_instance) {
            m_instance = new T();
        }

        return *m_instance;
    }

    template<typename T> Singleton<T>::Singleton()
        : m_instance(nullptr)
    { }

    template<typename T> void Singleton<T>::InternalDestroy() {
        if (!m_instance) {
            return;
        }

        m_instance->OnSingletonDestroy();

        delete m_instance;
        m_instance = nullptr;
    }
}

#include <EvoVulkan/Tools/VulkanDebug.h>
#include <EvoVulkan/Complexes/Shader.h>

/**
 * глупый компилятор может решить что некоторые из этих функций не нужны и выпилит их,
 * однако они будут использоваться в конечном приложении и будет ошибка линковки.
 * Данный метод не нужно ниоткуда вызывать. Достаточно прописать здесь функции.
*/
namespace EvoVulkan {
    EVK_MAYBE_UNUSED void CompileTimeFunctionsHolder() {
        EvoVulkan::Tools::VkDebug::Instance();
        EvoVulkan::Tools::VkDebug::Destroy();
        EvoVulkan::Complexes::GLSLCompiler::Instance();
        EvoVulkan::Complexes::GLSLCompiler::Destroy();
    }
}