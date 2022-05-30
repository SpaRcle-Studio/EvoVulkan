//
// Created by Monika on 18.07.2021.
//

#ifndef EVOVULKAN_FILESYSTEM_H
#define EVOVULKAN_FILESYSTEM_H

#include <EvoVulkan/Tools/StringUtils.h>

namespace EvoVulkan::Tools {
    static bool FileExists(const std::string& path) {
        struct stat buffer = {};
        return (stat(path.c_str(), &buffer) == 0);
    }

    static bool RemoveFile(const std::string& path) {
        return std::remove(path.c_str());
    }

    static bool CreateFolder(const std::string& directory) {
#ifdef EVK_MINGW
        return mkdir(directory.c_str()) == 0;
#else
        return _mkdir(directory.c_str()) == 0;
#endif
    }

    static bool Copy(const std::string& src, const std::string& dst) {
        if (!FileExists(src))
            return false;
        else {
            std::ifstream src_f(src, std::ios::binary);
            std::ofstream dst_f(dst, std::ios::binary);

            dst_f << src_f.rdbuf();

            src_f.close();
            dst_f.close();

            return true;
        }
    }

    //! path must be a fix
    static void CreatePath(std::string path, uint32_t offset = 0) {
        if (path.empty())
            return;

        if (path.back() != '/')
            path.append("/");

        auto pos = path.find('/', offset);
        if (pos != std::string::npos) {
            auto dir = Tools::Read(path, pos);
            Tools::CreateFolder(dir);
            CreatePath(path, pos + 1);
        }
    }
}

#endif //EVOVULKAN_FILESYSTEM_H
