#include "stdafx.h"

#include "Headers/FileManagement.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace Divide {

bool writeFile(const stringImpl& filePath, const stringImpl& fileName, const bufferPtr content, size_t length, FileType fileType) {

    if (!filePath.empty() && content != nullptr && length > 0) {
        if (!pathExists(filePath.c_str())) {
            if (!createDirectories(filePath.c_str())) {
                return false;
            }
        }

        std::ofstream outputFile((filePath + fileName).c_str(), fileType == FileType::BINARY
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

bool deleteFile(const stringImpl& filePath, const stringImpl& fileName) {
    if (fileName.empty()) {
        return false;
    }
    boost::filesystem::path file(filePath + fileName);
    boost::filesystem::remove(file);
    return true;
}

bool copyFile(const stringImpl& sourcePath, const stringImpl& sourceName, const stringImpl& targetPath, const stringImpl& targetName, bool overwrite) {
    if (sourceName.empty() || targetName.empty()) {
        return false;
    }

    if (!fileExists((sourcePath + sourceName).c_str())) {
        return false;
    }

    if (!overwrite && fileExists((targetPath + targetName).c_str())) {
        return false;
    }

    boost::filesystem::path destination(targetPath + targetName);
    boost::filesystem::path source(sourcePath + sourceName);
    boost::filesystem::copy_file(source, destination, boost::filesystem::copy_option::overwrite_if_exists);
    return true;
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
    return true;
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

std::string extractFilePathAndName(char* argv0) {
    boost::system::error_code ec;
    boost::filesystem::path p(boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
    return p.make_preferred().string();
}

}; //namespace Divide
