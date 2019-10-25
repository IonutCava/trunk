#include "stdafx.h"

#include "Headers/FileManagement.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {

bool writeFile(const Str256& filePath, const Str64& fileName, const bufferPtr content, size_t length, FileType fileType) {

    if (!filePath.empty() && content != nullptr && length > 0) {
        if (!pathExists(filePath.c_str())) {
            if (!createDirectories(filePath.c_str())) {
                return false;
            }
        }

        std::ofstream outputFile((filePath + fileName).c_str(), fileType == FileType::BINARY
                                                             ? std::ios::out | std::ios::binary
                                                             : std::ios::out);
        outputFile.write(reinterpret_cast<char*>(content), length);
        outputFile.close();
        return outputFile.good();
    }

    return false;
}

stringImpl stripQuotes(const stringImpl& input) {
    stringImpl ret = input;

    if (!input.empty()) {
        ret.erase(std::remove(std::begin(ret), std::end(ret), '\"'), std::end(ret));
    }

    return ret;
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

bool deleteFile(const Str256& filePath, const Str64& fileName) {
    if (fileName.empty()) {
        return false;
    }
    boost::filesystem::path file((filePath + fileName).c_str());
    boost::filesystem::remove(file);
    return true;
}

bool copyFile(const Str256& sourcePath, const Str64& sourceName, const Str256& targetPath, const Str64& targetName, bool overwrite) {
    if (sourceName.empty() || targetName.empty()) {
        return false;
    }

    if (!fileExists((sourcePath + sourceName).c_str())) {
        return false;
    }

    if (!overwrite && fileExists((targetPath + targetName).c_str())) {
        return false;
    }

    boost::filesystem::path destination((targetPath + targetName).c_str());
    boost::filesystem::path source((sourcePath + sourceName).c_str());
    boost::filesystem::copy_file(source, destination, boost::filesystem::copy_option::overwrite_if_exists);
    return true;
}

bool findFile(const Str256& filePath, const Str64& fileName, stringImpl& foundPath) {

    boost::filesystem::path dir_path(filePath.c_str());
    boost::filesystem::path file_name(fileName.c_str());

    const boost::filesystem::recursive_directory_iterator end;
    const auto it = std::find_if(boost::filesystem::recursive_directory_iterator(dir_path), end,
        [&file_name](const boost::filesystem::directory_entry& e) {
        return e.path().filename() == file_name;
    });
    if (it == end) {
        return false;
    }
    else {
        foundPath = it->path().string().c_str();
        return true;
    }
}

bool hasExtension(const Str256& filePath, const Str8& extension) {
    Str8 ext("." + extension);
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
                } else {
                    //ToDo: check if this recurse in subfolders actually works
                    deleteAllFiles(it->path().string().c_str(), extension);
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

        case CacheType::TEXTURES:
            return deleteAllFiles((Paths::g_cacheLocation + Paths::Textures::g_metadataLocation).c_str());
    }

    return false;
}

std::string extractFilePathAndName(char* argv0) {
    boost::system::error_code ec;
    boost::filesystem::path p(boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
    return p.make_preferred().string();
}

}; //namespace Divide
