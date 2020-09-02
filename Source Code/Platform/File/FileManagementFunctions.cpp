#include "stdafx.h"

#include "Core/Headers/StringHelper.h"
#include "Headers/FileManagement.h"

#include <filesystem>

namespace Divide {

bool writeFile(const char* filePath, const char* fileName, const bufferPtr content, const size_t length, const FileType fileType) {

    if (!Util::IsEmptyOrNull(filePath) && content != nullptr && length > 0) {
        if (!pathExists(filePath) && !CreateDirectories(filePath)) {
            return false;
        }

        std::ofstream outputFile(stringImpl{ filePath } +fileName,
                                 fileType == FileType::BINARY
                                           ? std::ios::out | std::ios::binary
                                           : std::ios::out);

        outputFile.write(static_cast<char*>(content), length);
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
    const auto targetPath = std::filesystem::path(input).lexically_normal();
    FileWithPath ret;
    ret._fileName = targetPath.filename().generic_string();
    ret._path = targetPath.parent_path().generic_string();
    return ret;
}

bool pathExists(const char* filePath) {
    return is_directory(std::filesystem::path(filePath));
}

bool createDirectory(const char* path) {
    if (!pathExists(path)) {
        const auto targetPath = std::filesystem::path(path);
        std::error_code ec = {};
        return create_directory(targetPath, ec);
    }

    return true;
}

bool fileExists(const char* filePathAndName) {
    return is_regular_file(std::filesystem::path(filePathAndName));
}

bool fileExists(const char* filePath, const char* fileName) {
    return is_regular_file(std::filesystem::path(stringImpl{ filePath } + fileName));
}

bool createFile(const char* filePathAndName, bool overwriteExisting) {
    if (overwriteExisting && fileExists(filePathAndName)) {
        return std::ofstream(filePathAndName, std::fstream::in | std::fstream::trunc).good();
    }

    CreateDirectories((const_sysInfo()._pathAndFilename._path + "/" + splitPathToNameAndLocation(filePathAndName)._path).c_str());

    return std::ifstream(filePathAndName, std::fstream::in).good();
}

bool openFile(const char* filePath, const char* fileName) {
    return openFile("", filePath, fileName);
}

bool openFile(const char* cmd, const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName) || !fileExists(filePath, fileName)) {
        return false;
    }

    constexpr std::array<std::string_view, 2> searchPattern = {
        "//", "\\"
    };

    const stringImpl file = "\"" + Util::ReplaceString({ const_sysInfo()._pathAndFilename._path + "/" + filePath + fileName }, searchPattern, "/", true) + "\"";

    if (strlen(cmd) == 0) {
        return CallSystemCmd(file.c_str(), "");
    }

    return CallSystemCmd(cmd, file.c_str());
}

bool deleteFile(const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName)) {
        return false;
    }
    const std::filesystem::path file(stringImpl{ filePath } +fileName);
    std::filesystem::remove(file);
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

    std::filesystem::copy_file(std::filesystem::path(source),
                               std::filesystem::path(target),
                               std::filesystem::copy_options::overwrite_existing);

    return true;
}

bool findFile(const char* filePath, const char* fileName, stringImpl& foundPath) {
    const std::filesystem::path dir_path(filePath);
    std::filesystem::path file_name(fileName);

    const std::filesystem::recursive_directory_iterator end;
    const auto it = std::find_if(std::filesystem::recursive_directory_iterator(dir_path), end,
        [&file_name](const std::filesystem::directory_entry& e) {
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

    if (pathExists(filePath)) {
        const std::filesystem::path pathIn(filePath);
        for (const auto& p : std::filesystem::directory_iterator(pathIn)) {
            try {
                if (std::filesystem::is_regular_file(p.status())) {
                    if (!extension || (extension != nullptr && p.path().extension() == extension)) {
                        if (std::filesystem::remove(p.path())) {
                            ret = true;
                        }
                    }
                } else {
                    //ToDo: check if this recurse in subfolders actually works
                    deleteAllFiles(p.path().string().c_str(), extension);
                }
            } catch (const std::exception &ex) {
                ACKNOWLEDGE_UNUSED(ex);
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

bool clearCache(const CacheType type) {
    Str64 cache;
    switch (type) {
        case CacheType::SHADER_TEXT: cache = Paths::Shaders::g_cacheLocationText; break;
        case CacheType::SHADER_BIN : cache = Paths::Shaders::g_cacheLocationBin; break;
        case CacheType::TERRAIN    : cache = Paths::g_terrainCacheLocation; break;
        case CacheType::MODELS     : cache = Paths::g_geometryCacheLocation; break;
        case CacheType::TEXTURES   : cache = Paths::Textures::g_metadataLocation; break;

        default: return false;
    }

    return deleteAllFiles(Paths::g_cacheLocation + cache.c_str());
}

std::string extractFilePathAndName(char* argv0) {
    std::error_code ec;
    auto currentPath = std::filesystem::current_path();
    currentPath.append(argv0);
    std::filesystem::path p(std::filesystem::canonical(currentPath, ec));
    return p.make_preferred().string();
}

}; //namespace Divide
