//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_STRINGUTILS_H
#define EVOVULKAN_STRINGUTILS_H

#include <EvoVulkan/macros.h>

namespace EvoVulkan::Tools {
    EVK_MAYBE_UNUSED static std::string Format(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        std::vector<char> v(1024);
        while (true) {
            va_list args2;
            va_copy(args2, args);
            int res = vsnprintf(v.data(), v.size(), fmt, args2);
            if ((res >= 0) && (res < static_cast<int>(v.size()))) {
                va_end(args);
                va_end(args2);
                return std::string(v.data());
            }
            size_t size;
            if (res < 0)
                size = v.size() * 2;
            else
                size = static_cast<size_t>(res) + 1;
            v.clear();
            v.resize(size);
            va_end(args2);
        }
    }

    EVK_MAYBE_UNUSED inline static std::string Read(const std::string& str, uint32_t count) {
        return str.substr(0, count);
    }

    EVK_MAYBE_UNUSED static std::string Replace(std::string str, const std::string& from, const std::string& to) {
        ret:
        size_t start_pos = str.find(from);
        if(start_pos == std::string::npos)
            return str;
        str.replace(start_pos, from.length(), to);
        goto ret;
    }

    EVK_MAYBE_UNUSED static std::string FixPath(const std::string& path) {
        return Tools::Replace(Tools::Replace(path, "\\", "/"), "//", "/");
    }

    EVK_MAYBE_UNUSED static inline bool Contains(const std::string& str, const std::string& subStr) {
        return str.find(subStr) != std::string::npos;
    }

    template<typename T> static std::string Combine(
            const std::vector<T>& vector,
            const std::function<std::string(const T& value, uint32_t i, bool last)>& combiner)
    {
        std::string result;

        const uint32_t size = static_cast<uint32_t>(vector.size());

        for (uint32_t i = 0; i < size; ++i) {
            result.append(combiner(vector[i], i, (i + 1) == size));
        }

        return result;
    }

    template<typename T> static std::string PointerToString(const T* pPtr) {
        auto&& string = Format("%p", (void*)pPtr);
        std::transform(string.begin(), string.end(), string.begin(), ::tolower);
        return string;
    }
}

#define EVK_FORMAT(x, ...) EvoVulkan::Tools::Format(x, __VA_ARGS__)

#endif //EVOVULKAN_STRINGUTILS_H
