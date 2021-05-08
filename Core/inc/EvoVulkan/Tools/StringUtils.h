//
// Created by Nikita on 08.05.2021.
//

#ifndef EVOVULKAN_STRINGUTILS_H
#define EVOVULKAN_STRINGUTILS_H

namespace EvoVulkan::Tools {
    static inline bool Contains(const std::string& str, const std::string& subStr) {
        return str.find(subStr) != std::string::npos;
    }
}

#endif //EVOVULKAN_STRINGUTILS_H
