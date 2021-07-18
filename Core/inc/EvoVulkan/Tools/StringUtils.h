//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_STRINGUTILS_H
#define EVOVULKAN_STRINGUTILS_H

#include <string>

namespace EvoVulkan::Tools {
    inline static std::string Read(const std::string& str, uint32_t count) {
        return str.substr(0, count);
    }

    static std::string Replace(std::string str, const std::string& from, const std::string& to) {
        ret:
        size_t start_pos = str.find(from);
        if(start_pos == std::string::npos)
            return str;
        str.replace(start_pos, from.length(), to);
        goto ret;
    }

    static std::string FixPath(const std::string& path) {
        return Tools::Replace(Tools::Replace(path, "\\", "/"), "//", "/");
    }

    static inline bool Contains(const std::string& str, const std::string& subStr) {
        return str.find(subStr) != std::string::npos;
    }
}

#endif //EVOVULKAN_STRINGUTILS_H
