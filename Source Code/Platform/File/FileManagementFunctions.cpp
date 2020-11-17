#include "stdafx.h"

#include "Core/Headers/StringHelper.h"
#include "Headers/FileManagement.h"
#include "Headers/ResourcePath.h"

#include <filesystem>

namespace Divide {

std::string getWorkingDirectory() {
    return std::filesystem::current_path().generic_string();
}


bool writeFile(const ResourcePath& filePath, const ResourcePath& fileName, const bufferPtr content, const size_t length, const FileType fileType) {
    return writeFile(filePath.c_str(), fileName.c_str(), content, length, fileType);
}

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

FileAndPath splitPathToNameAndLocation(const ResourcePath& input) {
    FileAndPath ret = splitPathToNameAndLocation(input.c_str());
    return ret;
}

FileAndPath splitPathToNameAndLocation(const char* input) {
    const auto targetPath = std::filesystem::path(input).lexically_normal();

    FileAndPath ret = {
      ResourcePath(targetPath.filename().generic_string()),
      ResourcePath(targetPath.parent_path().generic_string())
    };

    return ret;
}

std::filesystem::path asPath(const char* filePath) {
    auto path = std::filesystem::path(filePath);
    return path;
}

std::filesystem::path asPath(const std::string_view& filePath) {
    auto path = std::filesystem::path(filePath);
    return path;
}

bool pathCompare(const char* filePathA, const char* filePathB) {
    auto pathA = asPath(filePathA).lexically_normal();
    auto pathB = asPath(filePathB).lexically_normal();

    const bool ret = pathA.compare(pathB) == 0;
    if (!ret) {
        if (pathA.empty() || pathB.empty()) {
            return pathA.empty() == pathB.empty();
        }

        const bool pathATrailingSlash = filePathA[strlen(filePathA) - 1] == '/';
        const bool pathBTrailingSlash = filePathB[strlen(filePathB) - 1] == '/';
        if (pathATrailingSlash != pathBTrailingSlash) {
           if (pathATrailingSlash) {
               pathB += '/';
           } else {
               pathA += '/';
           }

           return pathA.compare(pathB) == 0;
        }

        return false;
    }
    
    return true;
}

bool pathExists(const ResourcePath& filePath) {
    const bool ret = pathExists(filePath.c_str());
    return ret;
}

bool pathExists(const char* filePath) {
    const bool ret = is_directory(std::filesystem::path(filePath));
    return ret;
}

bool createDirectory(const ResourcePath& path) {
    const bool ret = createDirectory(path.c_str());
    return ret;
}

bool createDirectory(const char* path) {
    if (!pathExists(path)) {
        const auto targetPath = std::filesystem::path(path);
        std::error_code ec = {};
        const bool ret = create_directory(targetPath, ec);
        return ret;
    }

    return true;
}

bool fileExists(const ResourcePath& filePathAndName) {
    const bool ret = fileExists(filePathAndName.c_str());
    return ret;
}

bool fileExists(const char* filePathAndName) {
    const bool ret = is_regular_file(std::filesystem::path(filePathAndName));
    return ret;
}

bool fileExists(const char* filePath, const char* fileName) {
    const bool ret = is_regular_file(std::filesystem::path(stringImpl{ filePath } + fileName));
    return ret;
}

bool createFile(const char* filePathAndName, const bool overwriteExisting) {
    if (overwriteExisting && fileExists(filePathAndName)) {
        const bool ret = std::ofstream(filePathAndName, std::fstream::in | std::fstream::trunc).good();
        return ret;
    }

    CreateDirectories((const_sysInfo()._workingDirectory + splitPathToNameAndLocation(filePathAndName).second.str()).c_str());

    const bool ret = std::ifstream(filePathAndName, std::fstream::in).good();
    return ret;
}

bool openFile(const ResourcePath& filePath, const ResourcePath& fileName) {
    const bool ret = openFile(filePath.c_str(), fileName.c_str());
    return ret;
}

bool openFile(const char* filePath, const char* fileName) {
    const bool ret = openFile("", filePath, fileName);
    return ret;
}

bool openFile(const char* cmd, const ResourcePath& filePath, const ResourcePath& fileName) {
    const bool ret = openFile(cmd, filePath.c_str(), fileName.c_str());
    return ret;
}

bool openFile(const char* cmd, const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName) || !fileExists(filePath, fileName)) {
        return false;
    }

    constexpr std::array<std::string_view, 2> searchPattern = {
        "//", "\\"
    };

    const stringImpl file = "\"" + Util::ReplaceString(
        ResourcePath{ const_sysInfo()._workingDirectory + filePath + fileName }.str(),
        searchPattern, 
        "/", 
        true) + "\"";

    if (strlen(cmd) == 0) {
        const bool ret = CallSystemCmd(file.c_str(), "");
        return ret;
    }

    const bool ret = CallSystemCmd(cmd, file.c_str());
    return ret;
}

bool deleteFile(const ResourcePath& filePath, const ResourcePath& fileName) {
    const bool ret = deleteFile(filePath.c_str(), fileName.c_str());
    return ret;
}

bool deleteFile(const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName)) {
        return false;
    }
    const std::filesystem::path file(stringImpl{ filePath } +fileName);
    std::filesystem::remove(file);
    return true;
}

bool copyFile(const ResourcePath& sourcePath, const ResourcePath&  sourceName, const ResourcePath&  targetPath, const ResourcePath& targetName, const bool overwrite) {
    const bool ret = copyFile(sourcePath.c_str(), sourceName.c_str(), targetPath.c_str(), targetName.c_str(), overwrite);
    return ret;
}

bool copyFile(const char* sourcePath, const char* sourceName, const char* targetPath, const char* targetName, const bool overwrite) {
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

    copy_file(std::filesystem::path(source),
              std::filesystem::path(target),
              std::filesystem::copy_options::overwrite_existing);

    return true;
}

bool findFile(const ResourcePath& filePath, const char* fileName, stringImpl& foundPath) {
    const bool ret = findFile(filePath.c_str(), fileName, foundPath);
    return ret;
}

bool findFile(const char* filePath, const char* fileName, stringImpl& foundPath) {
    const std::filesystem::path dir_path(filePath);
    std::filesystem::path file_name(fileName);

    const std::filesystem::recursive_directory_iterator end;
    const auto it = std::find_if(std::filesystem::recursive_directory_iterator(dir_path), end,
        [&file_name](const std::filesystem::directory_entry& e) {
        const bool ret = e.path().filename() == file_name;
        return ret;
    });
    if (it == end) {
        return false;
    } else {
        foundPath = it->path().string();
        return true;
    }
}

bool hasExtension(const ResourcePath& filePath, const Str16& extension) {
    const bool ret = hasExtension(filePath.c_str(), extension);
    return ret;
}

bool hasExtension(const char* filePath, const Str16& extension) {
    const Str16 ext("." + extension);
    const bool ret = Util::CompareIgnoreCase(Util::GetTrailingCharacters(stringImplFast{ filePath }, ext.length()), ext);
    return ret;
}

bool deleteAllFiles(const ResourcePath& filePath, const char* extension) {
    const bool ret = deleteAllFiles(filePath.c_str(), extension);
    return ret;
}

bool deleteAllFiles(const char* filePath, const char* extension) {
    bool ret = false;

    if (pathExists(filePath)) {
        const std::filesystem::path pathIn(filePath);
        for (const auto& p : std::filesystem::directory_iterator(pathIn)) {
            try {
                if (is_regular_file(p.status())) {
                    if (!extension || extension != nullptr && p.path().extension() == extension) {
                        if (std::filesystem::remove(p.path())) {
                            ret = true;
                        }
                    }
                } else {
                    //ToDo: check if this recurse in subfolders actually works
                    if (!deleteAllFiles(p.path().string().c_str(), extension)) {
                        NOP();
                    }
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
    ResourcePath cache;
    switch (type) {
        case CacheType::SHADER_TEXT: cache = Paths::Shaders::g_cacheLocationText; break;
        case CacheType::SHADER_BIN : cache = Paths::Shaders::g_cacheLocationBin; break;
        case CacheType::TERRAIN    : cache = Paths::g_terrainCacheLocation; break;
        case CacheType::MODELS     : cache = Paths::g_geometryCacheLocation; break;
        case CacheType::TEXTURES   : cache = Paths::Textures::g_metadataLocation; break;

        case CacheType::COUNT:
        default: return false;
    }

    const bool ret = deleteAllFiles(Paths::g_cacheLocation + cache);
    return ret;
}

std::string extractFilePathAndName(char* argv0) {
    std::error_code ec;
    auto currentPath = std::filesystem::current_path();
    currentPath.append(argv0);
    std::filesystem::path p(canonical(currentPath, ec));

    std::string ret = p.make_preferred().string();
    return ret;
}

}; //namespace Divide
