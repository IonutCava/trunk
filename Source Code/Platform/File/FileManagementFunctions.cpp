#include "stdafx.h"

#include "Headers/FileManagement.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {

bool writeFile(const char* filePath, const char* fileName, const bufferPtr content, size_t length, FileType fileType) {

    if (!Util::IsEmptyOrNull(filePath) && content != nullptr && length > 0) {
        if (!pathExists(filePath)) {
            if (!createDirectories(filePath)) {
                return false;
            }
        }

        std::ofstream outputFile(stringImpl{ filePath } +fileName,
                                 fileType == FileType::BINARY
                                           ? std::ios::out | std::ios::binary
                                           : std::ios::out);

        outputFile.write(reinterpret_cast<char*>(content), length);
        outputFile.close();
        return outputFile.good();
    }

    return false;
}

stringImpl stripQuotes(const char* input) {
    stringImpl ret = input;

    if (!ret.empty()) {
        ret.erase(std::remove(std::begin(ret), std::end(ret), '\"'), std::end(ret));
    }

    return ret;
}

FileWithPath splitPathToNameAndLocation(const char* input) {
    FileWithPath ret = {};
    ret._path = input;

    size_t pathNameSplitPoint = ret._path.find_last_of('/') + 1;
    if (pathNameSplitPoint == 0) {
        pathNameSplitPoint = ret._path.find_last_of("\\") + 1;
    }

    ret._fileName = ret._path.substr(pathNameSplitPoint, stringImpl::npos),
    ret._path = ret._path.substr(0, pathNameSplitPoint);
    return ret;
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

bool deleteFile(const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName)) {
        return false;
    }
    boost::filesystem::path file(stringImpl{ filePath } +fileName);
    boost::filesystem::remove(file);
    return true;
}

bool copyFile(const char* sourcePath, const char* sourceName, const char* targetPath, const char* targetName, bool overwrite) {
    if (Util::IsEmptyOrNull(sourceName) || Util::IsEmptyOrNull(targetName)) {
        return false;
    }
    stringImpl source{ sourcePath };
    source.append(sourceName);

    if (!fileExists(source.c_str())) {
        return false;
    }

    stringImpl target{ targetPath };
    target.append(targetName);

    if (!overwrite && fileExists(target.c_str())) {
        return false;
    }

    boost::filesystem::copy_file(boost::filesystem::path(source), boost::filesystem::path(target), boost::filesystem::copy_option::overwrite_if_exists);

    return true;
}

bool findFile(const char* filePath, const char* fileName, stringImpl& foundPath) {

    boost::filesystem::path dir_path(filePath);
    boost::filesystem::path file_name(fileName);

    const boost::filesystem::recursive_directory_iterator end;
    const auto it = std::find_if(boost::filesystem::recursive_directory_iterator(dir_path), end,
        [&file_name](const boost::filesystem::directory_entry& e) {
        return e.path().filename() == file_name;
    });
    if (it == end) {
        return false;
    } else {
        foundPath = it->path().string();
        return true;
    }
}

bool hasExtension(const char* filePath, const Str16& extension) {
    const Str16 ext("." + extension);
    return Util::CompareIgnoreCase(Util::GetTrailingCharacters(stringImplFast{ filePath }, ext.length()), ext);
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
           return deleteAllFiles(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText.c_str());
       
        case CacheType::SHADER_BIN:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin.c_str());

        case CacheType::TERRAIN:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::g_terrainCacheLocation.c_str());

        case CacheType::MODELS:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::g_geometryCacheLocation.c_str());

        case CacheType::TEXTURES:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::Textures::g_metadataLocation.c_str());
    }

    return false;
}

std::string extractFilePathAndName(char* argv0) {
    boost::system::error_code ec;
    boost::filesystem::path p(boost::filesystem::canonical(argv0, boost::filesystem::current_path(), ec));
    return p.make_preferred().string();
}

}; //namespace Divide
