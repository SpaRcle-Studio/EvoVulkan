//
// Created by Monika on 28.09.2021.
//

#ifndef EVOVULKAN_SINGLETON_H
#define EVOVULKAN_SINGLETON_H

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Tools {
    template<typename T>
    class DLL_EVK_EXPORT Singleton : public Tools::NonCopyable {
    protected:
        Singleton();
        ~Singleton() override = default;

    public:
        EVK_MAYBE_UNUSED static T& Instance();
        EVK_MAYBE_UNUSED static void Destroy();

    protected:
        virtual void OnSingletonDestroy() { }

    private:
        static Singleton<T>** GetSingleton();

        T& InternalInstance();
        void InternalDestroy();

    private:
        T* m_instance;

    };
}

#endif //EVOVULKAN_SINGLETON_H
