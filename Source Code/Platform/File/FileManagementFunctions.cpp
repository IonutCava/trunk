#include "stdafx.h"

#include "Headers/FileManagement.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {

bool writeFile(const stringImpl& filePath, const bufferPtr content, size_t length, FileType fileType) {
    if (!filePath.empty() && content != nullptr && length > 0) {
        std::ofstream outputFile(filePath.c_str(), fileType == FileType::BINARY
                                                             ? std::ios::out | std::ios::binary
                                                             : std::ios::out);
        outputFile.write(reinterpret_cast<Byte*>(content), length);
        outputFile.close();
        return outputFile.good();
    }

    return false;
}

FileWithPath splitPathToNameAndLocation(const stringImpl& input) {
    size_t pathNameSplitPoint = input.find_last_of('/') + 1;
    if (pathNameSplitPoint == 0) {
        pathNameSplitPoint = input.find_last_of("\\") + 1;
    }
    return FileWithPath
    {
        input.substr(pathNameSplitPoint, stringImpl::npos),
        input.substr(0, pathNameSplitPoint)
    };
}

//ref: https://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c
bool pathExists(const char* filePath) {
    struct stat info;

    if (stat(filePath, &info) != 0) {
        return false;
    } else if (info.st_mode & S_IFDIR) {
        return true;
    }

    return false;
}

bool fileExists(const char* filePathAndName) {
    return std::ifstream(filePathAndName).good();
}

bool createFile(const char* filePathAndName, bool overwriteExisting) {
    if (overwriteExisting && fileExists(filePathAndName)) {
        return std::ifstream(filePathAndName, std::fstream::in | std::fstream::trunc).good();
    }

    const SysInfo& systemInfo = const_sysInfo();
    createDirectories((systemInfo._pathAndFilename._path + "/" + splitPathToNameAndLocation(filePathAndName)._path).c_str());

    return std::ifstream(filePathAndName, std::fstream::in).good();

}

bool hasExtension(const stringImpl& filePath, const stringImpl& extension) {
    stringImpl ext("." + extension);
    return Util::CompareIgnoreCase(Util::GetTrailingCharacters(filePath, ext.length()), ext);
}

bool deleteAllFiles(const char* filePath, const char* extension) {
    bool ret = false;

    boost::filesystem::path p(filePath);
    if (boost::filesystem::exists(p) && boost::filesystem::is_directory(p)) {
        boost::filesystem::directory_iterator end;
        for (boost::filesystem::directory_iterator it(p); it != end; ++it) {
            try {
                if (boost::filesystem::is_regular_file(it->status())) {
                    if (!extension || (extension != nullptr && it->path().extension() == extension)) {
                        if (boost::filesystem::remove(it->path())) {
                            ret = true;
                        }
                    }
                }
            } catch (const std::exception &ex)
            {
                ex;
            }
        }
    }

    return ret;
}

bool clearCache() {
    for (U8 i = 0; i < to_U8(CacheType::COUNT); ++i) {
        if (!clearCache(static_cast<CacheType>(i))) {
            return false;
        }
    }
}

bool clearCache(CacheType type) {
    switch (type) {
        case CacheType::SHADER_TEXT:
           return deleteAllFiles((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText).c_str());
       
        case CacheType::SHADER_BIN:
            return deleteAllFiles((Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin).c_str());

        case CacheType::TERRAIN:
            return deleteAllFiles((Paths::g_cacheLocation + Paths::g_terrainCacheLocation).c_str());

        case CacheType::MODELS:
            return deleteAllFiles((Paths::g_cacheLocation + Paths::g_geometryCacheLocation).c_str());
    }

    return false;
}

}; //namespace Divide
