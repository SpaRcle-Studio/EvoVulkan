//
// Created by Monika on 18.07.2021.
//

#ifndef GAMEENGINE_FILESYSTEM_H
#define GAMEENGINE_FILESYSTEM_H

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include <EvoVulkan/Tools/StringUtils.h>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <direct.h>

namespace EvoVulkan::Tools {
    static bool FileExists(const std::string& path) {
        struct stat buffer = {};
        return (stat(path.c_str(), &buffer) == 0);
    }

    static bool RemoveFile(const std::string& path) {
        return std::remove(path.c_str());
    }

    static bool CreateFolder(const std::string& directory) {
#ifdef __MINGW64__
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
    static void CreatePath(const std::string& path, uint32_t offset = 0) {
        auto pos = path.find('/', offset);
        if (pos != std::string::npos) {
            auto dir = Tools::Read(path, pos);
            Tools::CreateFolder(dir);
            CreatePath(path, pos + 1);
        }
    }
}

#endif //GAMEENGINE_FILESYSTEM_H
